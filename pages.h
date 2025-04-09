const char LOGIN_HTML[] = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP32-CAM Login</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            body {
                font-family: Arial, sans-serif;
                margin: 20px;
                text-align: center;
                background-color: #f0f0f0;
            }
            .container {
                max-width: 800px;
                margin: 0 auto;
                background: white;
                padding: 20px;
                border-radius: 8px;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            }
            .form-group {
                margin: 15px 0;
            }
            input[type="text"], input[type="password"] {
                width: 100%;
                padding: 8px;
                margin: 5px 0;
                border: 1px solid #ddd;
                border-radius: 4px;
            }
            button {
                background-color: #4CAF50;
                color: white;
                padding: 10px 20px;
                border: none;
                border-radius: 4px;
                cursor: pointer;
                margin: 5px;
            }
            button:hover {
                background-color: #45a049;
            }
            .login-options {
                display: flex;
                justify-content: center;
                gap: 10px;
                margin: 20px 0;
            }
            .video-container {
                margin: 20px 0;
                display: none;
            }
            #stream {
                width: 100%;
                max-width: 800px;
                height: auto;
                border-radius: 4px;
            }
            .error-message {
                color: red;
                margin: 10px 0;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>ESP32-CAM Login</h1>
            
            <div class="login-options">
                <button onclick="showCredentialLogin()">Login with Credentials</button>
                <button onclick="showFaceLogin()">Login with Face</button>
            </div>
    
            <div id="credentialLogin">
                <div class="form-group">
                    <input type="text" id="username" placeholder="Username" required>
                </div>
                <div class="form-group">
                    <input type="password" id="password" placeholder="Password" required>
                </div>
                <button onclick="loginWithCredentials()">Login</button>
                <p>Don't have an account? <a href="/register">Register here</a></p>
            </div>
    
            <div id="faceLogin" style="display: none;">
                <div class="video-container" id="videoContainer">
                    <img id="stream" src="" alt="Loading camera stream...">
                </div>
                <button onclick="startFaceLogin()">Start Face Detection</button>
            </div>
    
            <div id="errorMessage" class="error-message"></div>
        </div>
    
        <script>
            var baseHost = document.location.origin;
            var streamUrl = baseHost + ':81/stream';
            var faceDetectionActive = false;

            // Check if already authenticated
            fetch(baseHost + '/check-auth')
                .then(response => {
                    if (response.ok) {
                        window.location.href = '/dashboard';
                    }
                });
    
            function showCredentialLogin() {
                document.getElementById('credentialLogin').style.display = 'block';
                document.getElementById('faceLogin').style.display = 'none';
                stopFaceDetection();
            }
    
            function showFaceLogin() {
                document.getElementById('credentialLogin').style.display = 'none';
                document.getElementById('faceLogin').style.display = 'block';
                document.getElementById('videoContainer').style.display = 'block';
                document.getElementById('stream').src = streamUrl;
            }
    
            function startFaceLogin() {
                faceDetectionActive = true;
                fetch(baseHost + '/control?var=face_detect&val=1')
                    .then(response => response.text())
                    .then(() => {
                        return fetch(baseHost + '/control?var=face_recognize&val=1');
                    })
                    .then(response => response.text())
                    .then(() => {
                        checkFaceRecognition();
                    });
            }
    
            function stopFaceDetection() {
                faceDetectionActive = false;
                fetch(baseHost + '/control?var=face_detect&val=0')
                    .then(() => {
                        fetch(baseHost + '/control?var=face_recognize&val=0');
                    });
            }
    
            function checkFaceRecognition() {
                if (!faceDetectionActive) return;
                
                fetch(baseHost + '/status')
                    .then(response => response.json())
                    .then(data => {
                        if (data.face_detect && data.face_recognized) {
                            window.location.href = '/dashboard';
                        } else {
                            setTimeout(checkFaceRecognition, 1000);
                        }
                    });
            }
    
            function loginWithCredentials() {
                const username = document.getElementById('username').value;
                const password = document.getElementById('password').value;
                
                if (!username || !password) {
                    document.getElementById('errorMessage').textContent = 'Please fill in all fields';
                    return;
                }
    
                fetch(baseHost + '/login', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ username, password })
                })
                .then(response => {
                    if (response.ok) {
                        window.location.href = '/dashboard';
                    } else {
                        document.getElementById('errorMessage').textContent = 'Invalid credentials';
                    }
                })
                .catch(error => {
                    document.getElementById('errorMessage').textContent = 'Login failed';
                });
            }
        </script>
    </body>
    </html>
)rawliteral";

const char DASHBOARD_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-CAM Dashboard</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Roboto', sans-serif;
            margin: 0;
            padding: 20px;
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            min-height: 100vh;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.95);
            padding: 25px;
            border-radius: 15px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
        }
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 30px;
            padding-bottom: 20px;
            border-bottom: 2px solid #eee;
        }
        h1 {
            margin: 0;
            color: #2c3e50;
            font-weight: 500;
        }
        .dashboard-grid {
            display: grid;
            grid-template-columns: 1fr 2fr;
            gap: 25px;
            margin-bottom: 25px;
        }
        .status-bar {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 15px;
            margin: 20px 0;
        }
        .status-item {
            background: #fff;
            padding: 15px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.07);
            text-align: center;
        }
        .status-item span {
            display: block;
            font-size: 1.2em;
            font-weight: 500;
            color: #3498db;
            margin-top: 5px;
        }
        .video-container {
            background: #000;
            border-radius: 15px;
            overflow: hidden;
            box-shadow: 0 8px 16px rgba(0,0,0,0.1);
            max-width: 480px;
            margin: 0 auto;
        }
        #stream {
            width: 100%;
            display: block;
        }
        button {
            background: #3498db;
            color: white;
            padding: 12px 25px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 14px;
            font-weight: 500;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        button:hover {
            background: #2980b9;
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0,0,0,0.15);
        }
        button.logout {
            background: #e74c3c;
        }
        button.logout:hover {
            background: #c0392b;
        }
        .controls {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 15px;
            margin: 20px 0;
        }
        .activity-log {
            background: #fff;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.07);
            height: 300px;
            overflow-y: auto;
        }
        .activity-item {
            padding: 10px;
            border-bottom: 1px solid #eee;
            color: #34495e;
            font-size: 14px;
        }
        .activity-item:last-child {
            border-bottom: none;
        }
        .modal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0,0,0,0.7);
            z-index: 1000;
            backdrop-filter: blur(5px);
        }
        .modal-content {
            background: white;
            margin: 5% auto;
            padding: 30px;
            width: 90%;
            max-width: 600px;
            border-radius: 15px;
            position: relative;
            box-shadow: 0 15px 35px rgba(0,0,0,0.2);
        }
        .close {
            position: absolute;
            right: 25px;
            top: 15px;
            font-size: 28px;
            font-weight: bold;
            cursor: pointer;
            color: #666;
            transition: color 0.3s;
        }
        .close:hover {
            color: #000;
        }
        .settings-grid, .user-form {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 20px;
            margin: 20px 0;
        }
        .setting-item {
            display: flex;
            flex-direction: column;
            gap: 8px;
        }
        .setting-item label {
            font-weight: 500;
            color: #2c3e50;
        }
        .setting-item select, .setting-item input, .user-form input {
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 6px;
            font-size: 14px;
        }
        .setting-item input[type="range"] {
            width: 100%;
        }
        /* User Management Modal Specific Styles */
        #addUserModal .modal-content {
            max-width: 800px;
        }
        .user-form {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        .user-form-inputs {
            padding: 20px;
        }
        .user-form-camera {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
        }
        .user-form-camera .video-container {
            max-width: 320px;
            margin-bottom: 15px;
        }
        .enrollment-controls {
            display: flex;
            gap: 10px;
            margin: 15px 0;
            flex-wrap: wrap;
            justify-content: center;
        }
        .enrollment-status {
            display: flex;
            gap: 15px;
            justify-content: center;
            margin-top: 15px;
        }
        .status-badge {
            padding: 5px 10px;
            border-radius: 15px;
            font-size: 12px;
            font-weight: 500;
        }
        .status-badge.active {
            background: #2ecc71;
            color: white;
        }        .status-badge.inactive {
            background: #e74c3c;
            color: white;
        }
        /* Servo control panel styles */
        .servo-control-panel {
            background: #fff;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.07);
            margin: 20px 0;
        }
        .servo-control-panel h3 {
            margin-top: 0;
            color: #2c3e50;
        }
        .servo-toggle {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-bottom: 15px;
        }
        .servo-buttons {
            display: flex;
            justify-content: space-between;
            gap: 10px;
        }
        .servo-btn {
            flex: 1;
        }
        /* Toggle switch */
        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }
        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
        }
        input:checked + .slider {
            background-color: #3498db;
        }
        input:focus + .slider {
            box-shadow: 0 0 1px #3498db;
        }
        input:checked + .slider:before {
            transform: translateX(26px);
        }
        .slider.round {
            border-radius: 34px;
        }
        .slider.round:before {
            border-radius: 50%;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ESP32-CAM Dashboard</h1>
            <button class="logout" onclick="logout()">Logout</button>
        </div>        <div class="status-bar">
            <div class="status-item">
                System Uptime: <span id="uptime">0s</span>
            </div>
            <div class="status-item">
                Face Detection: <span id="detectStatus">OFF</span>
            </div>
            <div class="status-item">
                Face Recognition: <span id="recognizeStatus">OFF</span>
            </div>
            <div class="status-item">
                Servo Position: <span id="servoPositionDisplay">90°</span>
            </div>
        </div>

        <div class="video-container">
            <img id="stream" src="" alt="Loading camera stream...">
        </div>        <div class="controls">
            <button onclick="toggleDetection()">Toggle Face Detection</button>
            <button onclick="toggleRecognition()">Toggle Face Recognition</button>
            <button id="captureBtn" onclick="capturePhoto()">Capture Photo</button>
            <button onclick="openSettings()">Camera Settings</button>
            <button onclick="window.location.href='/register'">Add New User</button>
        </div>

        <div class="servo-control-panel">
            <h3>Servo Control</h3>
            <div class="servo-toggle">
                <label class="switch">
                    <input type="checkbox" id="autoTrackingToggle" checked onchange="toggleAutoTracking()">
                    <span class="slider round"></span>
                </label>
                <span>Auto Face Tracking</span>
            </div>
            <div class="servo-buttons">
                <button onclick="moveServo('left')" class="servo-btn">◀ Left</button>
                <button onclick="centerServo()" class="servo-btn">Center</button>
                <button onclick="moveServo('right')" class="servo-btn">Right ▶</button>
            </div>
        </div>

        <div class="activity-log">
            <h3>Recent Activities</h3>
            <div id="activities"></div>
        </div>
    </div>

    <!-- Camera Settings Modal -->
    <div id="settingsModal" class="modal">
        <div class="modal-content">
            <span class="close" onclick="closeSettings()">&times;</span>
            <h2>Camera Settings</h2>
            <div class="settings-grid">
                <div class="setting-item">
                    <label for="framesize">Resolution</label>
                    <select id="framesize" onchange="updateCameraSetting('framesize', this.value)">
                        <option value="0">QQVGA(160x120)</option>
                        <option value="3">HQVGA(240x176)</option>
                        <option value="4">QVGA(320x240)</option>
                        <option value="5">CIF(400x296)</option>
                        <option value="6">VGA(640x480)</option>
                        <option value="8">SVGA(800x600)</option>
                    </select>
                </div>
                <div class="setting-item">
                    <label for="quality">Quality</label>
                    <input type="range" id="quality" min="4" max="63" value="10" 
                           onchange="updateCameraSetting('quality', this.value)">
                </div>
                <div class="setting-item">
                    <label for="brightness">Brightness</label>
                    <input type="range" id="brightness" min="-2" max="2" value="0" 
                           onchange="updateCameraSetting('brightness', this.value)">
                </div>
                <div class="setting-item">
                    <label for="contrast">Contrast</label>
                    <input type="range" id="contrast" min="-2" max="2" value="0" 
                           onchange="updateCameraSetting('contrast', this.value)">
                </div>
            </div>
        </div>
    </div>    <script>
        var baseHost = document.location.origin;
        var streamUrl = baseHost + ':81/stream';
        var detection = false;
        var recognition = false;
        var autoTracking = true;
        var servoPosition = 90; // Initial position

        document.getElementById('stream').src = streamUrl;

        // Settings Modal
        function openSettings() {
            document.getElementById('settingsModal').style.display = 'block';
        }

        function closeSettings() {
            document.getElementById('settingsModal').style.display = 'none';
        }

        window.onclick = function(event) {
            if (event.target == document.getElementById('settingsModal')) {
                closeSettings();
            }
        }

        function updateCameraSetting(setting, value) {
            fetch(baseHost + '/control?var=' + setting + '&val=' + value)
                .then(response => response.text())
                .catch(error => console.error('Error:', error));
        }

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
        }        function capturePhoto() {
            // Show loading indicator
            document.getElementById('captureBtn').textContent = 'Capturing...';
            document.getElementById('captureBtn').disabled = true;
            
            // Fetch the image
            fetch(baseHost + '/capture')
                .then(response => response.blob())
                .then(blob => {
                    // Create a download link
                    const url = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
                    a.style.display = 'none';
                    a.href = url;
                    a.download = 'esp32cam-capture-' + timestamp + '.jpg';
                    
                    // Append to the document, click it, and remove it
                    document.body.appendChild(a);
                    a.click();
                    window.URL.revokeObjectURL(url);
                    document.body.removeChild(a);
                    
                    // Reset button state
                    document.getElementById('captureBtn').textContent = 'Capture Photo';
                    document.getElementById('captureBtn').disabled = false;
                })
                .catch(error => {
                    console.error('Error capturing photo:', error);
                    document.getElementById('captureBtn').textContent = 'Capture Photo';
                    document.getElementById('captureBtn').disabled = false;
                });
        }

        function logout() {
            fetch(baseHost + '/logout')
                .then(() => {
                    window.location.href = '/';
                });
        }

        function formatTime(seconds) {
            const days = Math.floor(seconds / 86400);
            const hours = Math.floor((seconds % 86400) / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const secs = seconds % 60;
            
            if (days > 0) return `${days}d ${hours}h ${minutes}m ${secs}s`;
            if (hours > 0) return `${hours}h ${minutes}m ${secs}s`;
            if (minutes > 0) return `${minutes}m ${secs}s`;
            return `${secs}s`;
        }

        function updateStatus() {
            fetch(baseHost + '/status')
                .then(response => response.json())
                .then(data => {
                    // Update uptime
                    document.getElementById('uptime').textContent = formatTime(data.uptime);
                    
                    // Update activities
                    const activitiesHtml = data.activities.map(activity => 
                        `<div class="activity-item">
                            ${activity.message} (${formatTime(Math.floor(Date.now()/1000 - activity.timestamp))} ago)
                         </div>`
                    ).join('');
                    document.getElementById('activities').innerHTML = activitiesHtml;
                    
                    // Update status indicators
                    document.getElementById('detectStatus').textContent = data.face_detect ? 'ON' : 'OFF';
                    document.getElementById('recognizeStatus').textContent = data.face_recognize ? 'ON' : 'OFF';
                    
                    // Update camera settings in modal
                    document.getElementById('framesize').value = data.framesize;
                    document.getElementById('quality').value = data.quality;
                    document.getElementById('brightness').value = data.brightness;
                    document.getElementById('contrast').value = data.contrast;
                });
        }

        // Update status every 5 seconds
        setInterval(updateStatus, 5000);
        updateStatus(); // Initial update        // Check authentication status periodically
        function checkAuth() {
            fetch(baseHost + '/check-auth')
                .then(response => {
                    if (!response.ok) {
                        window.location.href = '/';
                    }
                });
            setTimeout(checkAuth, 30000); // Check every 30 seconds
        }

        checkAuth();

        // Servo control functions
        function toggleAutoTracking() {
            autoTracking = document.getElementById('autoTrackingToggle').checked;
            fetch(baseHost + '/control?var=servo_auto_track&val=' + (autoTracking ? '1' : '0'))
                .then(response => {
                    console.log('Auto tracking set to: ' + autoTracking);
                })
                .catch(error => console.error('Error:', error));
        }

        function moveServo(direction) {
            let step = direction === 'left' ? 10 : -10;
            fetch(baseHost + '/control?var=servo_move&val=' + step)
                .then(response => response.json())
                .then(data => {
                    if (data.servo_pos !== undefined) {
                        servoPosition = data.servo_pos;
                        document.getElementById('servoPositionDisplay').textContent = servoPosition + '°';
                    }
                })
                .catch(error => console.error('Error:', error));
        }

        function centerServo() {
            fetch(baseHost + '/control?var=servo_center&val=1')
                .then(response => response.json())
                .then(data => {
                    if (data.servo_pos !== undefined) {
                        servoPosition = data.servo_pos;
                        document.getElementById('servoPositionDisplay').textContent = servoPosition + '°';
                    }
                })
                .catch(error => console.error('Error:', error));
        }
    </script>
</body>
</html>
)rawliteral";

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
                background-color: #f0f0f0;
            }
            .container {
                max-width: 800px;
                margin: 0 auto;
                background: white;
                padding: 20px;
                border-radius: 8px;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            }
            .form-group {
                margin: 15px 0;
            }
            input[type="text"], input[type="password"], input[type="email"], input[type="tel"] {
                width: 100%;
                padding: 8px;
                margin: 5px 0;
                border: 1px solid #ddd;
                border-radius: 4px;
            }
            button {
                background-color: #4CAF50;
                color: white;
                padding: 10px 20px;
                border: none;
                border-radius: 4px;
                cursor: pointer;
                margin: 5px;
            }
            button:hover {
                background-color: #45a049;
            }
            .video-container {
                margin: 20px 0;
            }
            #stream {
                width: 100%;
                max-width: 800px;
                height: auto;
                border-radius: 4px;
            }
            .toggle-container {
                display: flex;
                justify-content: center;
                gap: 10px;
                margin: 15px 0;
            }
            .error-message {
                color: red;
                margin: 10px 0;
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
            <div class="form-group">
                <input type="email" id="email" placeholder="Email Address" required>
            </div>
            <div class="form-group">
                <input type="tel" id="phone" placeholder="Phone Number (for SMS alerts)" required>
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
            <div id="errorMessage" class="error-message"></div>
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
                const email = document.getElementById('email').value;
                const phone = document.getElementById('phone').value;
                
                if (!username || !password || !email || !phone) {
                    document.getElementById('errorMessage').textContent = 'Please fill in all fields';
                    return;
                }

                if (!email.match(/^[^\s@]+@[^\s@]+\.[^\s@]+$/)) {
                    document.getElementById('errorMessage').textContent = 'Please enter a valid email address';
                    return;
                }

                if (!phone.match(/^\+?[\d\s-]+$/)) {
                    document.getElementById('errorMessage').textContent = 'Please enter a valid phone number';
                    return;
                }
    
                if (!enrollment) {
                    document.getElementById('errorMessage').textContent = 'Please enable face enrollment and look at the camera';
                    return;
                }
    
                fetch(baseHost + '/register/submit', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ username, password, email, phone })
                })
                .then(response => {
                    if (response.ok) {
                        alert('Registration successful! You can now login.');
                        window.location.href = '/';
                    } else if (response.status === 503) {
                        document.getElementById('errorMessage').textContent = 'Maximum number of users reached';
                    } else {
                        document.getElementById('errorMessage').textContent = 'Registration failed';
                    }
                })
                .catch(error => {
                    document.getElementById('errorMessage').textContent = 'Registration failed';
                });
            }
        </script>
    </body>
    </html>
)rawliteral";
