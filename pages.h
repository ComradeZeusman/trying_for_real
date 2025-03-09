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
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
        }
        .status-bar {
            display: flex;
            justify-content: space-around;
            margin: 10px 0;
            padding: 10px;
            background: #f8f9fa;
            border-radius: 4px;
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
        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            margin: 5px;
        }
        button.logout {
            background-color: #f44336;
        }
        .controls {
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            gap: 10px;
            margin: 20px 0;
        }
        .activity-log {
            text-align: left;
            margin: 20px 0;
            padding: 10px;
            background: #f8f9fa;
            border-radius: 4px;
            max-height: 200px;
            overflow-y: auto;
        }
        .activity-item {
            padding: 5px 0;
            border-bottom: 1px solid #ddd;
        }
        .modal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0,0,0,0.5);
        }
        .modal-content {
            background: white;
            margin: 15% auto;
            padding: 20px;
            width: 80%;
            max-width: 500px;
            border-radius: 8px;
        }
        .close {
            float: right;
            cursor: pointer;
            font-size: 24px;
        }
        .settings-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 10px;
            margin: 20px 0;
        }
        .setting-item {
            display: flex;
            flex-direction: column;
            align-items: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ESP32-CAM Dashboard</h1>
            <button class="logout" onclick="logout()">Logout</button>
        </div>

        <div class="status-bar">
            <div>System Uptime: <span id="uptime">0s</span></div>
            <div>Face Detection: <span id="detectStatus">OFF</span></div>
            <div>Face Recognition: <span id="recognizeStatus">OFF</span></div>
        </div>

        <div class="video-container">
            <img id="stream" src="" alt="Loading camera stream...">
        </div>

        <div class="controls">
            <button onclick="toggleDetection()">Toggle Face Detection</button>
            <button onclick="toggleRecognition()">Toggle Face Recognition</button>
            <button onclick="capturePhoto()">Capture Photo</button>
            <button onclick="openSettings()">Camera Settings</button>
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
    </div>

    <script>
        var baseHost = document.location.origin;
        var streamUrl = baseHost + ':81/stream';
        var detection = false;
        var recognition = false;

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
        }

        function capturePhoto() {
            window.open(baseHost + '/capture');
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
        updateStatus(); // Initial update

        // Check authentication status periodically
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

            fetch(baseHost + '/register/submit', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ username, password })
            })
            .then(response => {
                if (response.ok) {
                    alert('Registration successful! You can now login.');
                    window.location.href = '/';
                } else if (response.status === 503) {
                    alert('Maximum number of users reached');
                } else {
                    alert('Registration failed');
                }
            })
            .catch(error => {
                alert('Registration failed');
            });
        }
    </script>
</body>
</html>
)rawliteral";

