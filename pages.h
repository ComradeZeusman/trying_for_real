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
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ESP32-CAM Dashboard</h1>
            <button class="logout" onclick="logout()">Logout</button>
        </div>

        <div class="video-container">
            <img id="stream" src="" alt="Loading camera stream...">
        </div>

        <div class="controls">
            <button onclick="toggleDetection()">Face Detection: <span id="detectStatus">OFF</span></button>
            <button onclick="toggleRecognition()">Face Recognition: <span id="recognizeStatus">OFF</span></button>
            <button onclick="capturePhoto()">Capture Photo</button>
        </div>
    </div>

    <script>
        var baseHost = document.location.origin;
        var streamUrl = baseHost + ':81/stream';
        var detection = false;
        var recognition = false;

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

        function capturePhoto() {
            window.open(baseHost + '/capture');
        }

        function logout() {
            fetch(baseHost + '/logout')
                .then(() => {
                    window.location.href = '/';
                });
        }

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

const char CAMERA_CONFIG_HTML[] = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP32-CAM Configuration</title>
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
            .config-section {
                margin: 20px 0;
                text-align: left;
                padding: 15px;
                border: 1px solid #ddd;
                border-radius: 4px;
            }
            .config-item {
                margin: 10px 0;
                display: flex;
                align-items: center;
                justify-content: space-between;
            }
            select, input[type="range"] {
                width: 200px;
                margin-left: 10px;
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
            .back-button {
                background-color: #666;
            }
            .value-display {
                min-width: 40px;
                display: inline-block;
                text-align: right;
                margin-left: 10px;
            }
            .preview-container {
                margin: 20px 0;
                text-align: center;
            }
            #stream {
                width: 100%;
                max-width: 400px;
                height: auto;
                border-radius: 4px;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <div class="header">
                <h1>Camera Configuration</h1>
                <button class="back-button" onclick="window.location.href='/dashboard'">Back to Dashboard</button>
            </div>
    
            <div class="preview-container">
                <h3>Live Preview</h3>
                <img id="stream" src="" alt="Loading camera stream...">
            </div>
    
            <div class="config-section">
                <h2>Image Settings</h2>
                <div class="config-item">
                    <label>Frame Size:</label>
                    <select id="framesize" onchange="updateValue('framesize', this.value)">
                        <option value="0">QQVGA (160x120)</option>
                        <option value="1">QVGA (320x240)</option>
                        <option value="2">VGA (640x480)</option>
                        <option value="3">SVGA (800x600)</option>
                        <option value="4">XGA (1024x768)</option>
                        <option value="5">SXGA (1280x1024)</option>
                    </select>
                </div>
                <div class="config-item">
                    <label>Quality (0-63):</label>
                    <input type="range" id="quality" min="0" max="63" onchange="updateValue('quality', this.value)">
                    <span id="quality-val" class="value-display">10</span>
                </div>
            </div>
    
            <div class="config-section">
                <h2>Camera Adjustments</h2>
                <div class="config-item">
                    <label>Brightness (-2 to 2):</label>
                    <input type="range" id="brightness" min="-2" max="2" step="1" onchange="updateValue('brightness', this.value)">
                    <span id="brightness-val" class="value-display">0</span>
                </div>
                <div class="config-item">
                    <label>Contrast (-2 to 2):</label>
                    <input type="range" id="contrast" min="-2" max="2" step="1" onchange="updateValue('contrast', this.value)">
                    <span id="contrast-val" class="value-display">0</span>
                </div>
                <div class="config-item">
                    <label>Saturation (-2 to 2):</label>
                    <input type="range" id="saturation" min="-2" max="2" step="1" onchange="updateValue('saturation', this.value)">
                    <span id="saturation-val" class="value-display">0</span>
                </div>
            </div>
    
            <div class="config-section">
                <h2>Special Effects</h2>
                <div class="config-item">
                    <label>Effect:</label>
                    <select id="special_effect" onchange="updateValue('special_effect', this.value)">
                        <option value="0">No Effect</option>
                        <option value="1">Negative</option>
                        <option value="2">Grayscale</option>
                        <option value="3">Red Tint</option>
                        <option value="4">Green Tint</option>
                        <option value="5">Blue Tint</option>
                        <option value="6">Sepia</option>
                    </select>
                </div>
            </div>
    
            <div class="config-section">
                <h2>Camera Controls</h2>
                <div class="config-item">
                    <label>AWB (Auto White Balance):</label>
                    <input type="checkbox" id="awb" onchange="updateValue('awb', this.checked ? 1 : 0)">
                </div>
                <div class="config-item">
                    <label>AWB Gain:</label>
                    <input type="checkbox" id="awb_gain" onchange="updateValue('awb_gain', this.checked ? 1 : 0)">
                </div>
                <div class="config-item">
                    <label>Horizontal Mirror:</label>
                    <input type="checkbox" id="hmirror" onchange="updateValue('hmirror', this.checked ? 1 : 0)">
                </div>
                <div class="config-item">
                    <label>Vertical Flip:</label>
                    <input type="checkbox" id="vflip" onchange="updateValue('vflip', this.checked ? 1 : 0)">
                </div>
            </div>
        </div>
    
        <script>
            var baseHost = document.location.origin;
            var streamUrl = baseHost + ':81/stream';
    
            document.getElementById('stream').src = streamUrl;
    
            // Initialize settings
            window.onload = function() {
                fetchCameraStatus();
            }
    
            function fetchCameraStatus() {
                fetch(baseHost + '/status')
                    .then(response => response.json())
                    .then(data => {
                        // Update all input values based on current camera status
                        document.getElementById('framesize').value = data.framesize;
                        document.getElementById('quality').value = data.quality;
                        document.getElementById('quality-val').textContent = data.quality;
                        document.getElementById('brightness').value = data.brightness;
                        document.getElementById('brightness-val').textContent = data.brightness;
                        document.getElementById('contrast').value = data.contrast;
                        document.getElementById('contrast-val').textContent = data.contrast;
                        document.getElementById('saturation').value = data.saturation;
                        document.getElementById('saturation-val').textContent = data.saturation;
                        document.getElementById('special_effect').value = data.special_effect;
                        document.getElementById('awb').checked = data.awb == 1;
                        document.getElementById('awb_gain').checked = data.awb_gain == 1;
                        document.getElementById('hmirror').checked = data.hmirror == 1;
                        document.getElementById('vflip').checked = data.vflip == 1;
                    });
            }
    
            function updateValue(variable, value) {
                fetch(baseHost + `/control?var=${variable}&val=${value}`)
                    .then(response => {
                        if (!response.ok) {
                            console.error('Failed to update:', variable);
                            return;
                        }
                        // Update value display if exists
                        const displayElement = document.getElementById(variable + '-val');
                        if (displayElement) {
                            displayElement.textContent = value;
                        }
                    })
                    .catch(error => console.error('Error:', error));
            }
    
            // Check authentication status periodically
            function checkAuth() {
                fetch(baseHost + '/check-auth')
                    .then(response => {
                        if (!response.ok) {
                            window.location.href = '/';
                        }
                    });
                setTimeout(checkAuth, 30000);
            }
    
            checkAuth();
        </script>
    </body>
    </html>
    )rawliteral";