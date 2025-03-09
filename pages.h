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