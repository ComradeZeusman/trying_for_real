const char REGISTRATION_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-CAM Registration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            text-align: center;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
        }
        .form-group {
            margin: 15px 0;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 8px;
            margin: 5px 0;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            cursor: pointer;
            margin: 5px;
        }
        .video-container {
            margin: 20px 0;
        }
        #stream {
            width: 100%;
            max-width: 800px;
            height: auto;
        }
        .toggle-container {
            display: flex;
            justify-content: center;
            gap: 10px;
            margin: 15px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-CAM Registration</h1>
        
        <div class="form-group">
            <input type="text" id="username" placeholder="Username" required>
        </div>
        <div class="form-group">
            <input type="password" id="password" placeholder="Password" required>
        </div>
        
        <div class="toggle-container">
            <button onclick="toggleDetection()">Face Detection: <span id="detectStatus">OFF</span></button>
            <button onclick="toggleRecognition()">Face Recognition: <span id="recognizeStatus">OFF</span></button>
            <button onclick="toggleEnrollment()">Face Enrollment: <span id="enrollStatus">OFF</span></button>
        </div>

        <div class="video-container">
            <img id="stream" src="" alt="Loading camera stream...">
        </div>

        <button onclick="registerUser()">Register</button>
    </div>

    <script>
        var baseHost = document.location.origin;
        var streamUrl = baseHost + ':81/stream';
        var detection = false;
        var recognition = false;
        var enrollment = false;

        document.getElementById('stream').src = streamUrl;

        function toggleDetection() {
            detection = !detection;
            fetch(baseHost + '/control?var=face_detect&val=' + (detection ? '1' : '0'))
                .then(response => {
                    document.getElementById('detectStatus').textContent = detection ? 'ON' : 'OFF';
                });
        }

        function toggleRecognition() {
            recognition = !recognition;
            fetch(baseHost + '/control?var=face_recognize&val=' + (recognition ? '1' : '0'))
                .then(response => {
                    document.getElementById('recognizeStatus').textContent = recognition ? 'ON' : 'OFF';
                });
        }

        function toggleEnrollment() {
            enrollment = !enrollment;
            fetch(baseHost + '/control?var=face_enroll&val=' + (enrollment ? '1' : '0'))
                .then(response => {
                    document.getElementById('enrollStatus').textContent = enrollment ? 'ON' : 'OFF';
                });
        }

        function registerUser() {
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;
            
            if (!username || !password) {
                alert('Please fill in all fields');
                return;
            }

            if (!enrollment) {
                alert('Please enable face enrollment and look at the camera');
                return;
            }

            // Here you would typically send the registration data to your server
            alert('Registration successful for user: ' + username);
        }
    </script>
</body>
</html>
)rawliteral";