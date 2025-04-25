// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "camera_index.h"
#include "Arduino.h"
#include "pages.h"  // Include our new pages header
#include <HTTPClient.h>
#include <WiFi.h>
#include <SPIFFS.h>  // Add SPIFFS library for file system
#include <ESP32Servo.h>

#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

// Face tracking variables - these need to be accessible from the main file
int faceX = -1;  // X coordinate of detected face center, -1 if no face
int faceY = -1;  // Y coordinate of detected face center
int faceWidth = 0; // Width of detected face
int faceHeight = 0; // Height of detected face
bool faceDetected = false; // Flag to indicate if a face is currently detected

// Buzzer control variables
bool buzzer_active = false; // Flag to track if buzzer is currently activated
unsigned long last_buzzer_api_call = 0; // Timestamp of last buzzer API call
unsigned long buzzer_activation_time = 0; // Timestamp when the buzzer was last activated
const unsigned long BUZZER_API_COOLDOWN = 5000; // 5 seconds cooldown between API calls
const unsigned long BUZZER_AUTO_TURNOFF_DELAY = 10000; // 10 seconds auto-turnoff

extern void moveServoToTrackFace(); // Forward declaration for function in main file

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

// Forward declarations
static void log_activity(const char* message);
static void control_buzzer(bool activate); // Function to make API requests for buzzer control
void check_buzzer_auto_turnoff(); // Function to check if buzzer should be turned off automatically (made public for use in main sketch)

// Add authentication related structures and variables
typedef struct {
    char username[32];
    char password[32];
    char email[64];
    char phone[20];
    uint8_t face_id;
} user_t;

#define MAX_USERS 10
static user_t users[MAX_USERS];
static int num_users = 0;
static bool is_authenticated = false;

// SPIFFS functions to save and load user data
static bool saveUsersToSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return false;
    }
    
    File file = SPIFFS.open("/users.dat", "w");
    if (!file) {
        Serial.println("Failed to open users file for writing");
        return false;
    }
    
    // Write number of users
    file.write((uint8_t*)&num_users, sizeof(num_users));
    
    // Write each user record
    for (int i = 0; i < num_users; i++) {
        file.write((uint8_t*)&users[i], sizeof(user_t));
    }
    
    file.close();
    Serial.printf("Saved %d users to SPIFFS\n", num_users);
    return true;
}

static bool loadUsersFromSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return false;
    }
    
    if (!SPIFFS.exists("/users.dat")) {
        Serial.println("No users file found, starting fresh");
        return false;
    }
    
    File file = SPIFFS.open("/users.dat", "r");
    if (!file) {
        Serial.println("Failed to open users file for reading");
        return false;
    }
    
    // Read number of users
    file.read((uint8_t*)&num_users, sizeof(num_users));
    
    // Safety check to avoid buffer overflow
    if (num_users > MAX_USERS) {
        num_users = MAX_USERS;
    }
    
    // Read each user record
    for (int i = 0; i < num_users; i++) {
        file.read((uint8_t*)&users[i], sizeof(user_t));
    }
    
    file.close();
    Serial.printf("Loaded %d users from SPIFFS\n", num_users);
    return true;
}

#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7

#define FACE_COLOR_WHITE  0x00FFFFFF
#define FACE_COLOR_BLACK  0x00000000
#define FACE_COLOR_RED    0x000000FF
#define FACE_COLOR_GREEN  0x0000FF00
#define FACE_COLOR_BLUE   0x00FF0000
#define FACE_COLOR_YELLOW (FACE_COLOR_RED | FACE_COLOR_GREEN)
#define FACE_COLOR_CYAN   (FACE_COLOR_BLUE | FACE_COLOR_GREEN)
#define FACE_COLOR_PURPLE (FACE_COLOR_BLUE | FACE_COLOR_RED)

#define MAX_ACTIVITIES 10
#define FLASH_LED_PIN 4

typedef struct {
    char message[100];
    unsigned long timestamp;
} activity_log_t;

static activity_log_t activities[MAX_ACTIVITIES];
static int activity_count = 0;
static unsigned long start_time = 0;
static bool activatesms = false;

typedef struct {
        size_t size; //number of values used for filtering
        size_t index; //current value index
        size_t count; //value count
        int sum;
        int * values; //array to be filled with values
} ra_filter_t;

typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static ra_filter_t ra_filter;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static mtmn_config_t mtmn_config = {0};
static int8_t detection_enabled = 0;
static int8_t recognition_enabled = 0;
static int8_t is_enrolling = 0;
static face_id_list id_list = {0};

// No buzzer variables needed

static ra_filter_t * ra_filter_init(ra_filter_t * filter, size_t sample_size){
    memset(filter, 0, sizeof(ra_filter_t));

    filter->values = (int *)malloc(sample_size * sizeof(int));
    if(!filter->values){
        return NULL;
    }
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}

static int ra_filter_run(ra_filter_t * filter, int value){
    if(!filter->values){
        return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size) {
        filter->count++;
    }
    return filter->sum / filter->count;
}

static void rgb_print(dl_matrix3du_t *image_matrix, uint32_t color, const char * str){
    fb_data_t fb;
    fb.width = image_matrix->w;
    fb.height = image_matrix->h;
    fb.data = image_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    fb_gfx_print(&fb, (fb.width - (strlen(str) * 14)) / 2, 10, color, str);
}

static int rgb_printf(dl_matrix3du_t *image_matrix, uint32_t color, const char *format, ...){
    char loc_buf[64];
    char * temp = loc_buf;
    int len;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(copy);
    if(len >= sizeof(loc_buf)){
        temp = (char*)malloc(len+1);
        if(temp == NULL) {
            return 0;
        }
    }
    vsnprintf(temp, len+1, format, arg);
    va_end(arg);
    rgb_print(image_matrix, color, temp);
    if(len > 64){
        free(temp);
    }
    return len;
}
static void setup_led() {
    pinMode(FLASH_LED_PIN, OUTPUT);
    digitalWrite(FLASH_LED_PIN, LOW);
}

static void flash_led() {
    for(int i = 0; i < 5; i++) {
        digitalWrite(FLASH_LED_PIN, HIGH);
        delay(100);
        digitalWrite(FLASH_LED_PIN, LOW);
        delay(100);
    }
}

static void control_buzzer(bool activate) {
    // Skip API calls if we've made one recently (to prevent flooding)
    if (millis() - last_buzzer_api_call < BUZZER_API_COOLDOWN) {
        return;
    }
    
    // Only make API call if buzzer state needs to change
    if (activate == buzzer_active) {
        return;
    }
    
    HTTPClient http;
    
    // Correct URL format (replacing semicolons with colons)
    if (activate) {
        http.begin("https://api-4u7e.onrender.com/run_buzzer");
        Serial.println("Sending API request to start buzzer");
    } else {
        http.begin("https://api-4u7e.onrender.com/stop_buzzer");
        Serial.println("Sending API request to stop buzzer");
    }
    
    // Make the GET request
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("Buzzer API response: %d - %s\n", httpResponseCode, response.c_str());        buzzer_active = activate;  // Update buzzer status
        
        // Log the activity
        if (activate) {
            buzzer_activation_time = millis(); // Record when buzzer was activated
            log_activity("Buzzer activated due to intruder detection");
        } else {
            log_activity("Buzzer deactivated - no intruder detected");
        }
    } else {
        Serial.printf("Buzzer API request failed, error: %d\n", httpResponseCode);
    }
    
    // Record time of API call
    last_buzzer_api_call = millis();
    
    http.end();
}

static void log_activity(const char* message) {
    if (activity_count < MAX_ACTIVITIES) {
        activity_count++;
    }
    // Shift activities
    for (int i = MAX_ACTIVITIES - 1; i > 0; i--) {
        memcpy(&activities[i], &activities[i-1], sizeof(activity_log_t));
    }
    strncpy(activities[0].message, message, sizeof(activities[0].message) - 1);
    activities[0].timestamp = millis();
}

void check_buzzer_auto_turnoff() {
    // Check if buzzer is active and if the auto-turnoff time has passed
    if (buzzer_active && (millis() - buzzer_activation_time >= BUZZER_AUTO_TURNOFF_DELAY)) {
        Serial.println("Auto-turnoff: Buzzer has been active for 10 seconds, turning off");
        control_buzzer(false);
    }
}

static void send_sms_alert(const char* phone_number) {
    // HTTPClient http;
    // http.begin("https://telcomw.com/api-v2/send");
    // http.addHeader("Content-Type", "multipart/form-data; boundary=boundary");
    
    // // Use a static buffer to build the body
    // char body[512]; // Adjust size based on your needs
    // snprintf(body, sizeof(body),
    //     "--boundary\r\n"
    //     "Content-Disposition: form-data; name=\"api_key\"\r\n\r\nEBNZQ2IHYOP6MQXMI0UF\r\n"
    //     "--boundary\r\n"
    //     "Content-Disposition: form-data; name=\"password\"\r\n\r\niamwhoiam123\r\n"
    //     "--boundary\r\n"
    //     "Content-Disposition: form-data; name=\"text\"\r\n\r\nIntruder Alert! Unknown face detected on your ESP32-CAM\r\n"
    //     "--boundary\r\n"
    //     "Content-Disposition: form-data; name=\"numbers\"\r\n\r\n%s\r\n"
    //     "--boundary\r\n"
    //     "Content-Disposition: form-data; name=\"from\"\r\n\r\nWGIT\r\n"
    //     "--boundary--\r\n",
    //     phone_number);

    // int httpResponseCode = http.POST((uint8_t*)body, strlen(body));
    
    // if (httpResponseCode > 0) {
    //     Serial.printf("SMS alert sent successfully, response code: %d\n", httpResponseCode);
    // } else {
    //     Serial.printf("Error sending SMS alert: %d\n", httpResponseCode);
    // }
    
    // http.end();
}

static void draw_face_boxes(dl_matrix3du_t *image_matrix, box_array_t *boxes, int face_id){
    int x, y, w, h, i;
    uint32_t color = FACE_COLOR_YELLOW;   
     if(face_id < 0){
        color = FACE_COLOR_RED;
        flash_led(); // Flash LED for intruder
        
    } else if(face_id > 0){
        color = FACE_COLOR_GREEN;
    }
    fb_data_t fb;
    fb.width = image_matrix->w;
    fb.height = image_matrix->h;
    fb.data = image_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    
    // Update face detection status - false by default, set to true if any face is found
    faceDetected = false;
    
    for (i = 0; i < boxes->len; i++){
        // rectangle box
        x = (int)boxes->box[i].box_p[0];
        y = (int)boxes->box[i].box_p[1];
        w = (int)boxes->box[i].box_p[2] - x + 1;
        h = (int)boxes->box[i].box_p[3] - y + 1;
        
        // Store face position data for servo tracking
        // Only track the first face (i==0) if multiple faces are detected
        if (i == 0) {
            faceX = x + w/2;  // Center X of face
            faceY = y + h/2;  // Center Y of face
            faceWidth = w;
            faceHeight = h;
            faceDetected = true;
            
            // Call the face tracking function to move the servo
            moveServoToTrackFace();
        }
        
        fb_gfx_drawFastHLine(&fb, x, y, w, color);
        fb_gfx_drawFastHLine(&fb, x, y+h-1, w, color);
        fb_gfx_drawFastVLine(&fb, x, y, h, color);
        fb_gfx_drawFastVLine(&fb, x+w-1, y, h, color);
#if 0
        // landmark
        int x0, y0, j;
        for (j = 0; j < 10; j+=2) {
            x0 = (int)boxes->landmark[i].landmark_p[j];
            y0 = (int)boxes->landmark[i].landmark_p[j+1];
            fb_gfx_fillRect(&fb, x0, y0, 3, 3, color);
        }
#endif
    }
}

static int run_face_recognition(dl_matrix3du_t *image_matrix, box_array_t *net_boxes){
    dl_matrix3du_t *aligned_face = NULL;
    int matched_id = 0;

    aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
    if(!aligned_face){
        Serial.println("Could not allocate face recognition buffer");
        return matched_id;
    }
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK){
        if (is_enrolling == 1){
            int8_t left_sample_face = enroll_face(&id_list, aligned_face);

            if(left_sample_face == (ENROLL_CONFIRM_TIMES - 1)){
                Serial.printf("Enrolling Face ID: %d\n", id_list.tail);
                char msg[100];
                snprintf(msg, sizeof(msg), "Started enrolling new face ID: %d", id_list.tail);
                log_activity(msg);
            }
            Serial.printf("Enrolling Face ID: %d sample %d\n", id_list.tail, ENROLL_CONFIRM_TIMES - left_sample_face);
            rgb_printf(image_matrix, FACE_COLOR_CYAN, "ID[%u] Sample[%u]", id_list.tail, ENROLL_CONFIRM_TIMES - left_sample_face);
            if (left_sample_face == 0){
                is_enrolling = 0;
                Serial.printf("Enrolled Face ID: %d\n", id_list.tail);
                char msg[100];
                snprintf(msg, sizeof(msg), "Successfully enrolled new face ID: %d", id_list.tail);
                log_activity(msg);
            }
        } else {
            matched_id = recognize_face(&id_list, aligned_face);     
                   if (matched_id >= 0) {
                Serial.printf("Match Face ID: %u\n", matched_id);
                rgb_printf(image_matrix, FACE_COLOR_GREEN, "Hello Subject %u", matched_id);
                char msg[100];
                snprintf(msg, sizeof(msg), "Recognized face ID: %d", matched_id);
                log_activity(msg);
                
                // Deactivate buzzer if it was active (recognized user, not an intruder)
                control_buzzer(false); 
            } else {
                Serial.println("No Match Found");
                rgb_print(image_matrix, FACE_COLOR_RED, "Intruder Alert!");
                log_activity("Intruder Alert - Unknown face detected");
                
                // Activate buzzer via API call
                control_buzzer(true);
                
                // Send SMS alert to all registered users
                if(activatesms){
                for(int i = 0; i < num_users; i++) {
                    if(users[i].phone[0] != '\0') {
                        send_sms_alert(users[i].phone);
                    }
                }
            }
                matched_id = -1;
            }
        }
    } else {
        Serial.println("Face Not Aligned");
        //rgb_print(image_matrix, FACE_COLOR_YELLOW, "Human Detected");
    }

    dl_matrix3du_free(aligned_face);
    return matched_id;
}

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t capture_handler(httpd_req_t *req){
    // Check if buzzer should be turned off automatically
    check_buzzer_auto_turnoff();
    
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    size_t out_len, out_width, out_height;
    uint8_t * out_buf;
    bool s;
    bool detected = false;
    int face_id = 0;
    if(!detection_enabled || fb->width > 400){
        size_t fb_len = 0;
        if(fb->format == PIXFORMAT_JPEG){
            fb_len = fb->len;
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        } else {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
        esp_camera_fb_return(fb);
        int64_t fr_end = esp_timer_get_time();
        Serial.printf("JPG: %uB %ums\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start)/1000));
        return res;
    }

    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
    if (!image_matrix) {
        esp_camera_fb_return(fb);
        Serial.println("dl_matrix3du_alloc failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    out_buf = image_matrix->item;
    out_len = fb->width * fb->height * 3;
    out_width = fb->width;
    out_height = fb->height;

    s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
    esp_camera_fb_return(fb);
    if(!s){
        dl_matrix3du_free(image_matrix);
        Serial.println("to rgb888 failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);

    if (net_boxes){
        detected = true;
        if(recognition_enabled){
            face_id = run_face_recognition(image_matrix, net_boxes);
        }
        draw_face_boxes(image_matrix, net_boxes, face_id);
        free(net_boxes->score);
        free(net_boxes->box);
        free(net_boxes->landmark);
        free(net_boxes);
    }

    jpg_chunking_t jchunk = {req, 0};
    s = fmt2jpg_cb(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);
    dl_matrix3du_free(image_matrix);
    if(!s){
        Serial.println("JPEG compression failed");
        return ESP_FAIL;
    }

    int64_t fr_end = esp_timer_get_time();
    Serial.printf("FACE: %uB %ums %s%d\n", (uint32_t)(jchunk.len), (uint32_t)((fr_end - fr_start)/1000), detected?"DETECTED ":"", face_id);
    return res;
}

static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    dl_matrix3du_t *image_matrix = NULL;
    bool detected = false;
    int face_id = 0;
    int64_t fr_start = 0;
    int64_t fr_ready = 0;
    int64_t fr_face = 0;
    int64_t fr_recognize = 0;
    int64_t fr_encode = 0;

    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");    while(true){
        // Check if buzzer should be turned off automatically
        check_buzzer_auto_turnoff();
        
        detected = false;
        face_id = 0;
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            fr_start = esp_timer_get_time();
            fr_ready = fr_start;
            fr_face = fr_start;
            fr_encode = fr_start;
            fr_recognize = fr_start;
            if(!detection_enabled || fb->width > 400){
                if(fb->format != PIXFORMAT_JPEG){
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if(!jpeg_converted){
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                } else {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            } else {

                image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);

                if (!image_matrix) {
                    Serial.println("dl_matrix3du_alloc failed");
                    res = ESP_FAIL;
                } else {
                    if(!fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item)){
                        Serial.println("fmt2rgb888 failed");
                        res = ESP_FAIL;
                    } else {
                        fr_ready = esp_timer_get_time();
                        box_array_t *net_boxes = NULL;
                        if(detection_enabled){
                            net_boxes = face_detect(image_matrix, &mtmn_config);
                        }
                        fr_face = esp_timer_get_time();
                        fr_recognize = fr_face;
                        if (net_boxes || fb->format != PIXFORMAT_JPEG){
                            if(net_boxes){
                                detected = true;
                                if(recognition_enabled){
                                    face_id = run_face_recognition(image_matrix, net_boxes);
                                }
                                fr_recognize = esp_timer_get_time();
                                draw_face_boxes(image_matrix, net_boxes, face_id);
                                free(net_boxes->score);
                                free(net_boxes->box);
                                free(net_boxes->landmark);
                                free(net_boxes);
                            }
                            if(!fmt2jpg(image_matrix->item, fb->width*fb->height*3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len)){
                                Serial.println("fmt2jpg failed");
                                res = ESP_FAIL;
                            }
                            esp_camera_fb_return(fb);
                            fb = NULL;
                        } else {
                            _jpg_buf = fb->buf;
                            _jpg_buf_len = fb->len;
                        }
                        fr_encode = esp_timer_get_time();
                    }
                    dl_matrix3du_free(image_matrix);
                }
            }
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();

        int64_t ready_time = (fr_ready - fr_start)/1000;
        int64_t face_time = (fr_face - fr_ready)/1000;
        int64_t recognize_time = (fr_recognize - fr_face)/1000;
        int64_t encode_time = (fr_encode - fr_recognize)/1000;
        int64_t process_time = (fr_encode - fr_start)/1000;
        
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
        Serial.printf("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps), %u+%u+%u+%u=%u %s%d\n",
            (uint32_t)(_jpg_buf_len),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
            avg_frame_time, 1000.0 / avg_frame_time,
            (uint32_t)ready_time, (uint32_t)face_time, (uint32_t)recognize_time, (uint32_t)encode_time, (uint32_t)process_time,
            (detected)?"DETECTED ":"", face_id
        );
    }

    last_frame = 0;
    return res;
}

static esp_err_t cmd_handler(httpd_req_t *req){
    char*  buf;
    size_t buf_len;
    char variable[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;

    if(!strcmp(variable, "framesize")) {
        if(s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
    }
    else if(!strcmp(variable, "quality")) res = s->set_quality(s, val);
    else if(!strcmp(variable, "contrast")) res = s->set_contrast(s, val);
    else if(!strcmp(variable, "brightness")) res = s->set_brightness(s, val);
    else if(!strcmp(variable, "saturation")) res = s->set_saturation(s, val);
    else if(!strcmp(variable, "gainceiling")) res = s->set_gainceiling(s, (gainceiling_t)val);
    else if(!strcmp(variable, "colorbar")) res = s->set_colorbar(s, val);
    else if(!strcmp(variable, "awb")) res = s->set_whitebal(s, val);
    else if(!strcmp(variable, "agc")) res = s->set_gain_ctrl(s, val);
    else if(!strcmp(variable, "aec")) res = s->set_exposure_ctrl(s, val);
    else if(!strcmp(variable, "hmirror")) res = s->set_hmirror(s, val);
    else if(!strcmp(variable, "vflip")) res = s->set_vflip(s, val);
    else if(!strcmp(variable, "awb_gain")) res = s->set_awb_gain(s, val);
    else if(!strcmp(variable, "agc_gain")) res = s->set_agc_gain(s, val);
    else if(!strcmp(variable, "aec_value")) res = s->set_aec_value(s, val);
    else if(!strcmp(variable, "aec2")) res = s->set_aec2(s, val);
    else if(!strcmp(variable, "dcw")) res = s->set_dcw(s, val);
    else if(!strcmp(variable, "bpc")) res = s->set_bpc(s, val);
    else if(!strcmp(variable, "wpc")) res = s->set_wpc(s, val);
    else if(!strcmp(variable, "raw_gma")) res = s->set_raw_gma(s, val);
    else if(!strcmp(variable, "lenc")) res = s->set_lenc(s, val);
    else if(!strcmp(variable, "special_effect")) res = s->set_special_effect(s, val);
    else if(!strcmp(variable, "wb_mode")) res = s->set_wb_mode(s, val);
    else if(!strcmp(variable, "ae_level")) res = s->set_ae_level(s, val);
    else if(!strcmp(variable, "face_detect")) {
        detection_enabled = val;
        if(!detection_enabled) {
            recognition_enabled = 0;
        }
    }
    else if(!strcmp(variable, "face_enroll")) is_enrolling = val;
    else if(!strcmp(variable, "face_recognize")) {
        recognition_enabled = val;
        if(recognition_enabled){
            detection_enabled = val;
        }
    }
    else if(!strcmp(variable, "servo_auto_track")) {
        // Enable/disable automatic face tracking
        extern bool autoTrackingEnabled;
        autoTrackingEnabled = val > 0;
        char msg[100];
        snprintf(msg, sizeof(msg), "Auto face tracking %s", autoTrackingEnabled ? "enabled" : "disabled");
        log_activity(msg);
    }
    else if(!strcmp(variable, "servo_move")) {
        // Command format: /control?var=servo_move&val=pan,tilt
        // Example: /control?var=servo_move&val=10,-5 (move pan right 10 degrees, tilt up 5 degrees)
        int pan_change = 0, tilt_change = 0;
        if (sscanf(value, "%d,%d", &pan_change, &tilt_change) == 2) {
            extern int panPosition;
            extern int tiltPosition;
            extern void setServoPositions(int pan, int tilt);
            
            // Calculate new positions
            int newPanPos = panPosition + pan_change;
            int newTiltPos = tiltPosition + tilt_change;
            
            // Update servo positions
            setServoPositions(newPanPos, newTiltPos);
            
            char msg[100];
            snprintf(msg, sizeof(msg), "Servo moved to pan:%d° tilt:%d°", panPosition, tiltPosition);
            log_activity(msg);
            
            // Prepare JSON response with current servo positions
            char json_response[100];
            snprintf(json_response, sizeof(json_response), 
                "{\"pan_pos\":%d,\"tilt_pos\":%d}", 
                panPosition, tiltPosition);
            httpd_resp_set_type(req, "application/json");
            return httpd_resp_send(req, json_response, strlen(json_response));
        }
    }    else if(!strcmp(variable, "servo_center")) {
        // Center both servos (move to 90 degrees)
        extern int panPosition;
        extern int tiltPosition;
        extern void setServoPositions(int pan, int tilt);
        
        setServoPositions(90, 90);
        log_activity("Servos centered to 90°");
        
        // Prepare JSON response
        char json_response[100];
        snprintf(json_response, sizeof(json_response), 
            "{\"pan_pos\":%d,\"tilt_pos\":%d}", 
            panPosition, tiltPosition);
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, json_response, strlen(json_response));
    }    else if(!strcmp(variable, "flash")) {
        // Control the flash LED using the already defined FLASH_LED_PIN macro
        digitalWrite(FLASH_LED_PIN, val > 0 ? HIGH : LOW);
        
        char msg[100];
        snprintf(msg, sizeof(msg), "Flash LED turned %s", val > 0 ? "ON" : "OFF");
        log_activity(msg);
        
        return httpd_resp_send(req, "OK", 2);
    }
    else {
        res = -1;
    }

    if(res){
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req){
    // Check if buzzer should be turned off automatically
    check_buzzer_auto_turnoff();
    
    static char json_response[2048];  // Increased buffer size

    sensor_t * s = esp_camera_sensor_get();
    char * p = json_response;
    *p++ = '{';

    // Add system uptime
    p+=sprintf(p, "\"uptime\":%lu,", millis() / 1000);

    // Add activities array
    p+=sprintf(p, "\"activities\":[");
    for(int i = 0; i < activity_count && i < MAX_ACTIVITIES; i++) {
        if(i > 0) p+=sprintf(p, ",");
        p+=sprintf(p, "{\"message\":\"%s\",\"timestamp\":%lu}", 
            activities[i].message, 
            activities[i].timestamp / 1000);
    }
    p+=sprintf(p, "],");

    // Existing camera status parameters
    p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p+=sprintf(p, "\"quality\":%u,", s->status.quality);
    p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p+=sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p+=sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p+=sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p+=sprintf(p, "\"awb\":%u,", s->status.awb);
    p+=sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p+=sprintf(p, "\"aec\":%u,", s->status.aec);
    p+=sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p+=sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p+=sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p+=sprintf(p, "\"agc\":%u,", s->status.agc);
    p+=sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p+=sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p+=sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p+=sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p+=sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p+=sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p+=sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p+=sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p+=sprintf(p, "\"dcw\":%u,", s->status.dcw);    p+=sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
    p+=sprintf(p, "\"face_detect\":%u,", detection_enabled);
    p+=sprintf(p, "\"face_enroll\":%u,", is_enrolling);
    p+=sprintf(p, "\"face_recognize\":%u,", recognition_enabled);
    
    // Add buzzer info
    p+=sprintf(p, "\"buzzer_active\":%s,", buzzer_active ? "true" : "false");
    if (buzzer_active) {
        unsigned long buzzer_active_time = (millis() - buzzer_activation_time) / 1000;
        unsigned long auto_off_in = (BUZZER_AUTO_TURNOFF_DELAY / 1000) - buzzer_active_time;
        p+=sprintf(p, "\"buzzer_active_for\":%lu,", buzzer_active_time);
        p+=sprintf(p, "\"buzzer_auto_off_in\":%lu", auto_off_in > 0 ? auto_off_in : 0);
    } else {
        p+=sprintf(p, "\"buzzer_active_for\":0,");
        p+=sprintf(p, "\"buzzer_auto_off_in\":0");
    }
    
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t index_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    if (is_authenticated) {
        return httpd_resp_send(req, DASHBOARD_HTML, strlen(DASHBOARD_HTML));
    } else {
        return httpd_resp_send(req, LOGIN_HTML, strlen(LOGIN_HTML));
    }
}

static esp_err_t login_handler(httpd_req_t *req) {
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        return ESP_FAIL;
    }
    content[recv_size] = '\0';

    char username[32];
    char password[32];
    // Basic parsing of JSON - in real app use proper JSON parser
    if (sscanf(content, "{\"username\":\"%31[^\"]\",\"password\":\"%31[^\"]\"}", username, password) == 2) {
        for (int i = 0; i < num_users; i++) {
            if (strcmp(users[i].username, username) == 0 && 
                strcmp(users[i].password, password) == 0) {
                is_authenticated = true;
                activatesms = true; // Enable SMS alerts for this user
                httpd_resp_set_status(req, "200 OK");
                httpd_resp_send(req, NULL, 0);
                return ESP_OK;
            }
        }
    }
    
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t register_handler(httpd_req_t *req) {
    if (num_users >= MAX_USERS) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_send(req, "Maximum users reached", 0);
        return ESP_OK;
    }

    char content[256];  // Increased buffer size for additional fields
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        return ESP_FAIL;
    }
    content[recv_size] = '\0';

    char username[32];
    char password[32];
    char email[64];
    char phone[20];
    
    if (sscanf(content, "{\"username\":\"%31[^\"]\",\"password\":\"%31[^\"]\",\"email\":\"%63[^\"]\",\"phone\":\"%19[^\"]\"}", 
        username, password, email, phone) == 4) {
        
        strncpy(users[num_users].username, username, sizeof(users[num_users].username) - 1);
        strncpy(users[num_users].password, password, sizeof(users[num_users].password) - 1);
        strncpy(users[num_users].email, email, sizeof(users[num_users].email) - 1);
        strncpy(users[num_users].phone, phone, sizeof(users[num_users].phone) - 1);
        users[num_users].face_id = id_list.tail;
        num_users++;
        
        saveUsersToSPIFFS();  // Save updated user list to SPIFFS
        
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t logout_handler(httpd_req_t *req) {
    is_authenticated = false;
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t check_auth_handler(httpd_req_t *req) {
    if (is_authenticated) {
        httpd_resp_set_status(req, "200 OK");
    } else {
        httpd_resp_set_status(req, "401 Unauthorized");
    }
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t dashboard_handler(httpd_req_t *req) {
    if (!is_authenticated) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // Check if this is a capture request
    char* buf = NULL;
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
      if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if (!buf) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char action[32] = {0};
            if (httpd_query_key_value(buf, "action", action, sizeof(action)) == ESP_OK) {
                if (strcmp(action, "capture") == 0) {
                    free(buf);
                    // This is a capture request, handle it
                    log_activity("Photo captured from dashboard");
                    return capture_handler(req);
                }
                else if (strcmp(action, "status") == 0) {
                    free(buf);
                    // This is a status request, forward to status handler
                    return status_handler(req);
                }
                else if (strcmp(action, "report") == 0) {
                    free(buf);
                    // This is a report generation request
                    log_activity("System report generated");
                    
                    // Get the system's sensor for camera config
                    sensor_t * s = esp_camera_sensor_get();
                    
                    // Create a buffer for the HTML report content
                    char *report_html = (char*)malloc(8192); // Allocate 8KB for the report
                    if (!report_html) {
                        httpd_resp_send_500(req);
                        return ESP_FAIL;
                    }
                    
                    // Start building the HTML report
                    int len = 0;
                    len += sprintf(report_html + len, 
                        "<!DOCTYPE html>\n"
                        "<html>\n"
                        "<head>\n"
                        "  <title>Ufulu Home Security System Report</title>\n"
                        "  <style>\n"
                        "    body { font-family: Arial, sans-serif; margin: 20px; }\n"
                        "    h1 { color: #3498db; text-align: center; }\n"
                        "    h2 { color: #2c3e50; border-bottom: 1px solid #eee; padding-bottom: 5px; }\n"
                        "    .section { margin: 20px 0; padding: 15px; background: #f8f9fa; border-radius: 5px; }\n"
                        "    table { width: 100%%; border-collapse: collapse; }\n"
                        "    th, td { padding: 10px; text-align: left; border-bottom: 1px solid #ddd; }\n"
                        "    th { background-color: #f2f2f2; }\n"
                        "    .status-good { color: green; }\n"
                        "    .status-warning { color: orange; }\n"
                        "    .status-danger { color: red; }\n"
                        "    .footer { text-align: center; margin-top: 30px; color: #7f8c8d; font-size: 12px; }\n"
                        "  </style>\n"
                        "</head>\n"
                        "<body>\n"
                        "  <h1>Ufulu Home Security System Report</h1>\n"
                        "  <div class='section'>\n"
                        "    <h2>System Information</h2>\n"
                        "    <table>\n"
                        "      <tr><th>Item</th><th>Value</th></tr>\n"
                        "      <tr><td>System Uptime</td><td>%lu seconds</td></tr>\n"
                        "      <tr><td>Camera Model</td><td>ESP32-CAM AI-THINKER</td></tr>\n"
                        "      <tr><td>Face Detection</td><td>%s</td></tr>\n"
                        "      <tr><td>Face Recognition</td><td>%s</td></tr>\n"
                        "      <tr><td>Registered Users</td><td>%d</td></tr>\n"
                        "    </table>\n"
                        "  </div>\n",
                        millis() / 1000,
                        detection_enabled ? "Enabled" : "Disabled",
                        recognition_enabled ? "Enabled" : "Disabled",
                        num_users
                    );
                    
                    // Add camera configuration section
                    len += sprintf(report_html + len,
                        "  <div class='section'>\n"
                        "    <h2>Camera Configuration</h2>\n"
                        "    <table>\n"
                        "      <tr><th>Setting</th><th>Value</th></tr>\n"
                        "      <tr><td>Resolution</td><td>%s</td></tr>\n"
                        "      <tr><td>Quality</td><td>%u</td></tr>\n"
                        "      <tr><td>Brightness</td><td>%d</td></tr>\n"
                        "      <tr><td>Contrast</td><td>%d</td></tr>\n"
                        "      <tr><td>Saturation</td><td>%d</td></tr>\n"
                        "      <tr><td>Horizontal Mirror</td><td>%s</td></tr>\n"
                        "      <tr><td>Vertical Flip</td><td>%s</td></tr>\n"
                        "    </table>\n"
                        "  </div>\n",
                        s->status.framesize == 0 ? "QQVGA (160x120)" :
                        s->status.framesize == 3 ? "HQVGA (240x176)" :
                        s->status.framesize == 4 ? "QVGA (320x240)" :
                        s->status.framesize == 5 ? "CIF (400x296)" :
                        s->status.framesize == 6 ? "VGA (640x480)" :
                        s->status.framesize == 8 ? "SVGA (800x600)" : "Unknown",
                        s->status.quality,
                        s->status.brightness,
                        s->status.contrast,
                        s->status.saturation,
                        s->status.hmirror ? "Yes" : "No",
                        s->status.vflip ? "Yes" : "No"
                    );
                    
                    // Add activity logs section
                    len += sprintf(report_html + len,
                        "  <div class='section'>\n"
                        "    <h2>Recent Activities</h2>\n"
                        "    <table>\n"
                        "      <tr><th>Event</th><th>Time (seconds ago)</th></tr>\n"
                    );
                    
                    for (int i = 0; i < activity_count && i < MAX_ACTIVITIES; i++) {
                        len += sprintf(report_html + len,
                            "      <tr><td>%s</td><td>%lu</td></tr>\n",
                            activities[i].message,
                            (millis() - activities[i].timestamp) / 1000
                        );
                    }
                    
                    len += sprintf(report_html + len, "    </table>\n  </div>\n");
                    
                    // Add servo and hardware section
                    extern int panPosition;
                    extern int tiltPosition;
                    extern bool autoTrackingEnabled;
                    
                    len += sprintf(report_html + len,
                        "  <div class='section'>\n"
                        "    <h2>Hardware Status</h2>\n"
                        "    <table>\n"
                        "      <tr><th>Device</th><th>Status</th></tr>\n"
                        "      <tr><td>Pan Servo Position</td><td>%d°</td></tr>\n"
                        "      <tr><td>Tilt Servo Position</td><td>%d°</td></tr>\n"
                        "      <tr><td>Auto Face Tracking</td><td>%s</td></tr>\n"
                        "      <tr><td>Alert Buzzer</td><td>%s</td></tr>\n"
                        "      <tr><td>Flash LED</td><td>Ready</td></tr>\n"
                        "    </table>\n"
                        "  </div>\n",
                        panPosition,
                        tiltPosition,
                        autoTrackingEnabled ? "Enabled" : "Disabled",
                        buzzer_active ? "Active" : "Ready"
                    );
                    
                    // Add SMS alerts section if applicable
                    len += sprintf(report_html + len,
                        "  <div class='section'>\n"
                        "    <h2>SMS Alert Recipients</h2>\n"
                        "    <table>\n"
                        "      <tr><th>User</th><th>Phone Number</th></tr>\n"
                    );
                    
                    // List users with phone numbers for SMS alerts
                    bool has_sms_recipients = false;
                    for (int i = 0; i < num_users; i++) {
                        if (users[i].phone[0] != '\0') {
                            has_sms_recipients = true;
                            len += sprintf(report_html + len,
                                "      <tr><td>%s</td><td>%s</td></tr>\n",
                                users[i].username,
                                users[i].phone
                            );
                        }
                    }
                    
                    if (!has_sms_recipients) {
                        len += sprintf(report_html + len,
                            "      <tr><td colspan='2'>No SMS recipients configured</td></tr>\n"
                        );
                    }
                    
                    len += sprintf(report_html + len, "    </table>\n  </div>\n");
                    
                    // Add intruder alert statistics
                    len += sprintf(report_html + len,
                        "  <div class='section'>\n"
                        "    <h2>Security Incidents</h2>\n"
                        "    <p>Note: This section shows detected security events from system logs.</p>\n"
                        "    <table>\n"
                        "      <tr><th>Incident Type</th><th>Count</th></tr>\n"
                    );
                    
                    // Count intrusion events from activity logs
                    int intruder_count = 0;
                    for (int i = 0; i < activity_count; i++) {
                        if (strstr(activities[i].message, "Intruder") != NULL) {
                            intruder_count++;
                        }
                    }
                    
                    len += sprintf(report_html + len,
                        "      <tr><td>Intruder Detections</td><td>%d</td></tr>\n"
                        "    </table>\n"
                        "  </div>\n",
                        intruder_count
                    );                    // Finalize the report with footer and print button
                    len += sprintf(report_html + len,
                        "  <div class='footer'>\n"
                        "    <p>Report generated at: %lu (system time in milliseconds)</p>\n"
                        "    <p>Ufulu Home Security System - ESP32-CAM</p>\n"
                        "  </div>\n"
                        "  <div style='text-align:center; margin:30px;'>\n"
                        "    <button onclick='window.print()' style='padding:10px 20px; background:#3498db; color:white; border:none; border-radius:4px; cursor:pointer; font-size:16px;'>Save as PDF</button>\n"
                        "  </div>\n"
                        "  <script>\n"
                        "    window.onload = function() {\n"
                        "      const timestamp = new Date().toISOString().replace(/[:.]/g, '-');\n"
                        "      document.title = 'ufulu-security-report-' + timestamp;\n"
                        "    }\n"
                        "  </script>\n"
                        "</body>\n"
                        "</html>",
                        millis()
                    );
                    
                    // Send as HTML content with proper headers
                    httpd_resp_set_type(req, "text/html");
                    // Don't set Content-Disposition header for inline viewing
                    esp_err_t res = httpd_resp_send(req, report_html, len);
                    
                    // Free the buffer
                    free(report_html);
                    return res;
                }
            }
        }
        free(buf);
    }
    
    // Normal dashboard request
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, DASHBOARD_HTML, strlen(DASHBOARD_HTML));
}

static esp_err_t registration_page_handler(httpd_req_t *req) {
     if (!is_authenticated) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, REGISTRATION_HTML, strlen(REGISTRATION_HTML));
}

void startCameraServer(){
    start_time = millis();  // Initialize start time
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.stack_size = 8192;

    // Initialize flash LED
    setup_led();
    
    // Load users from SPIFFS
    loadUsersFromSPIFFS();

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t registration_page_uri = {
        .uri       = "/register",
        .method    = HTTP_GET,
        .handler   = registration_page_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t register_api_uri = {
        .uri       = "/register/submit",
        .method    = HTTP_POST,
        .handler   = register_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t login_uri = {
        .uri       = "/login",
        .method    = HTTP_POST,
        .handler   = login_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t logout_uri = {
        .uri       = "/logout",
        .method    = HTTP_POST,
        .handler   = logout_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t check_auth_uri = {
        .uri       = "/check-auth",
        .method    = HTTP_GET,
        .handler   = check_auth_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t dashboard_uri = {
        .uri       = "/dashboard",
        .method    = HTTP_GET,
        .handler   = dashboard_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t status_uri = {
        .uri       = "/status",
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t capture_uri = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
    };

   httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    ra_filter_init(&ra_filter, 20);
    
    mtmn_config.type = FAST;
    mtmn_config.min_face = 80;
    mtmn_config.pyramid = 0.707;
    mtmn_config.pyramid_times = 4;
    mtmn_config.p_threshold.score = 0.6;
    mtmn_config.p_threshold.nms = 0.7;
    mtmn_config.p_threshold.candidate_number = 20;
    mtmn_config.r_threshold.score = 0.7;
    mtmn_config.r_threshold.nms = 0.7;
    mtmn_config.r_threshold.candidate_number = 10;
    mtmn_config.o_threshold.score = 0.7;
    mtmn_config.o_threshold.nms = 0.7;
    mtmn_config.o_threshold.candidate_number = 1;
    
    face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
    
    Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &registration_page_uri);
        httpd_register_uri_handler(camera_httpd, &register_api_uri);
        httpd_register_uri_handler(camera_httpd, &login_uri);
        httpd_register_uri_handler(camera_httpd, &logout_uri);
        httpd_register_uri_handler(camera_httpd, &check_auth_uri);
        httpd_register_uri_handler(camera_httpd, &dashboard_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        httpd_register_uri_handler(camera_httpd, &capture_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}