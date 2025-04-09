#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "soc/soc.h"   
#include "soc/rtc_cntl_reg.h"
#include <ESP32Servo.h>
#include <SPIFFS.h>
//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

// Face tracking variables defined in app_httpd.cpp
extern int faceX;       // X coordinate of detected face center
extern int faceY;       // Y coordinate of detected face center
extern int faceWidth;   // Width of detected face
extern int faceHeight;  // Height of detected face
extern bool faceDetected; // Flag to indicate if face is detected

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

// Define the servo constants
static const int servoPin = 14; // GPIO pin connected to the servo
Servo servo1;

// Servo parameters - these need to be non-static so they can be accessed from app_httpd.cpp
int servoPosition = 90;  // Track the current position of the servo (start at center position)
const int SERVO_STEP = 2;     // Degrees to move per step (smaller = smoother, but slower)
int SERVO_MIN = 10;     // Minimum allowed angle to prevent mechanical issues
int SERVO_MAX = 170;    // Maximum allowed angle
const int CENTER_X_THRESHOLD = 40; // How many pixels from center before we move the servo

// Flag to control whether servo auto-tracks faces
bool autoTrackingEnabled = true;  // Default to auto tracking enabled

// Camera frame parameters
const int CAMERA_CENTER_X = 160; // Assuming QVGA (320x240) with X center at 160

// Smoothing parameters - to prevent jittery movement
const int SMOOTHING_WINDOW = 5;
int xPositions[5] = {0, 0, 0, 0, 0};
int xPositionIndex = 0;

const char* ssid = "tama";
const char* password = "12345678";

void startCameraServer();

// Function to move servo to track the detected face
void moveServoToTrackFace() {
  if (!faceDetected) {
    return; // No face to track
  }
  
  // Store the X position in our smoothing array
  xPositions[xPositionIndex] = faceX;
  xPositionIndex = (xPositionIndex + 1) % SMOOTHING_WINDOW;
  
  // Calculate the average X position from our smoothing array
  int avgX = 0;
  for (int i = 0; i < SMOOTHING_WINDOW; i++) {
    avgX += xPositions[i];
  }
  avgX /= SMOOTHING_WINDOW;
  
  // Calculate the offset from center of the frame
  int offset = CAMERA_CENTER_X - avgX;
  
  // Only move if the offset is larger than the threshold
  if (abs(offset) > CENTER_X_THRESHOLD) {
    // Face is to the left of center, move servo clockwise
    if (offset > 0 && servoPosition < SERVO_MAX) {
      servoPosition += SERVO_STEP;
    } 
    // Face is to the right of center, move servo counterclockwise
    else if (offset < 0 && servoPosition > SERVO_MIN) {
      servoPosition -= SERVO_STEP;
    }
    
    // Constrain servo position to valid range
    servoPosition = constrain(servoPosition, SERVO_MIN, SERVO_MAX);
    
    // Update the servo position
    servo1.write(servoPosition);
    
    // Debug output
    Serial.print("Face detected at X: ");
    Serial.print(faceX);
    Serial.print(", Offset: ");
    Serial.print(offset);
    Serial.print(", Servo position: ");
    Serial.println(servoPosition);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  // Initialize SPIFFS for user data storage
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
  } else {
    Serial.println("SPIFFS mounted successfully");
  }
  
  // Initialize servo with specific PWM properties
  ESP32PWM::allocateTimer(1); // Use timer 0 for servo
  servo1.setPeriodHertz(50);  // Standard 50Hz servo
  servo1.attach(servoPin, 500, 2400); // Min/Max pulse width for most servos
  
  // Move servo to initial center position
  servo1.write(servoPosition);
  delay(500);
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  
  // Initialize face tracking
  for (int i = 0; i < SMOOTHING_WINDOW; i++) {
    xPositions[i] = CAMERA_CENTER_X;  // Initialize with center position
  }
}

// Function that can be called from app_httpd.cpp to set the servo position
void setServoPosition(int position) {
  servoPosition = constrain(position, SERVO_MIN, SERVO_MAX);
  servo1.write(servoPosition);
}

void loop() {
  // The face detection and tracking mainly happens in the HTTP server task
  // Inside the draw_face_boxes() function, which calls moveServoToTrackFace()
  
  // Reset face detection if no face is detected for a while
  static unsigned long lastFaceTime = 0;
  static bool wasDetected = false;
  
  if (faceDetected) {
    if (!wasDetected) {
      Serial.println("Face detected, tracking activated");
      wasDetected = true;
    }
    lastFaceTime = millis();
  } else if (wasDetected && (millis() - lastFaceTime > 3000)) {
    // No face detected for 3 seconds, return to center position
    Serial.println("Face lost, returning to center position");
    servoPosition = 90;
    servo1.write(servoPosition);
    wasDetected = false;
    
    // Reset smoothing array
    for (int i = 0; i < SMOOTHING_WINDOW; i++) {
      xPositions[i] = CAMERA_CENTER_X;
    }
  }
  
  // Small delay to prevent CPU hogging
  delay(50);
}