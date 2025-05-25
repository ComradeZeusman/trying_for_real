const char LOGIN_HTML[] = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>SecurEye Pro Security Login</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            body {
                font-family: Arial, sans-serif;
                margin: 20px;
                text-align: center;
                background-color: #121f2d;
                color: #eaeaea;
            }
            .container {
                max-width: 800px;
                margin: 0 auto;
                background: #212d3a;
                padding: 20px;
                border-radius: 8px;
                box-shadow: 0 4px 8px rgba(0,0,0,0.3);
            }
            .form-group {
                margin: 15px 0;
            }
            input[type="text"], input[type="password"] {
                width: 100%;
                padding: 8px;
                margin: 5px 0;
                border: 1px solid #455a74;
                border-radius: 4px;
                background-color: #2a3950;
                color: #eaeaea;
            }
            button {
                background-color: #1e88e5;
                color: white;
                padding: 10px 20px;
                border: none;
                border-radius: 4px;
                cursor: pointer;
                margin: 5px;
            }
            button:hover {
                background-color: #0d47a1;
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
                color: #ff5252;
                margin: 10px 0;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>SecurEye Pro Security Login</h1>    
            <div id="credentialLogin">
                <div class="form-group">
                    <input type="text" id="username" placeholder="Username" required>
                </div>
                <div class="form-group">
                    <input type="password" id="password" placeholder="Password" required>
                </div>
                <button onclick="loginWithCredentials()">Login</button>
            </div>

            <div id="errorMessage" class="error-message"></div>
        </div>

        <script>
            var baseHost = document.location.origin;

            // Check if already authenticated
            fetch(baseHost + '/check-auth')
                .then(response => {
                    if (response.ok) {
                        window.location.href = '/dashboard';
                    }
                });
    
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
    <title>SecurEye Pro Security Dashboard</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Roboto', sans-serif;
            margin: 0;
            padding: 20px;
            background: #121f2d;
            min-height: 100vh;
            color: #eaeaea;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: rgba(33, 45, 58, 0.95);
            padding: 25px;
            border-radius: 15px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.4);
        }
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 30px;
            padding-bottom: 20px;
            border-bottom: 2px solid #2a3950;
        }
        h1, h2, h3 {
            margin: 0;
            color: #1e88e5;
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
            background: #182635;
            padding: 15px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.15);
            text-align: center;
        }
        .status-item span {
            display: block;
            font-size: 1.2em;
            font-weight: 500;
            color: #1e88e5;
            margin-top: 5px;
        }
        .video-container {
            background: #000;
            border-radius: 15px;
            overflow: hidden;
            box-shadow: 0 8px 16px rgba(0,0,0,0.2);
            max-width: 480px;
            margin: 0 auto;
            border: 2px solid #1e88e5;
        }
        #stream {
            width: 100%;
            display: block;
        }
        button {
            background: #1e88e5;
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
            background: #0d47a1;
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0,0,0,0.3);
        }
        button.logout {
            background: #e53935;
        }
        button.logout:hover {
            background: #b71c1c;
        }
        .controls {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 15px;
            margin: 20px 0;
        }
        .activity-log {
            background: #182635;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.15);
            height: 300px;
            overflow-y: auto;
        }
        .activity-item {
            padding: 10px;
            border-bottom: 1px solid #2a3950;
            color: #eaeaea;
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
            background: rgba(0,0,0,0.85);
            z-index: 1000;
            backdrop-filter: blur(5px);
        }
        .modal-content {
            background: #182635;
            margin: 5% auto;
            padding: 30px;
            width: 90%;
            max-width: 600px;
            border-radius: 15px;
            position: relative;
            box-shadow: 0 15px 35px rgba(0,0,0,0.4);
        }
        .close {
            position: absolute;
            right: 25px;
            top: 15px;
            font-size: 28px;
            font-weight: bold;
            cursor: pointer;
            color: #888;
            transition: color 0.3s;
        }
        .close:hover {
            color: #fff;
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
            color: #eaeaea;
        }
        .setting-item select, .setting-item input, .user-form input {
            padding: 10px;
            border: 1px solid #2a3950;
            border-radius: 6px;
            font-size: 14px;
            background: #212d3a;
            color: #eaeaea;
        }
        .setting-item input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #2a3950;
            outline: none;
            -webkit-appearance: none;
        }
        .setting-item input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 18px;
            height: 18px;
            background: #1e88e5;
            border-radius: 50%;
            cursor: pointer;
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
            background: #212d3a;
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
            background: #43a047;
            color: white;
        }
        .status-badge.inactive {
            background: #e53935;
            color: white;
        }
        /* Servo control panel styles - Updated for sliders */
        .servo-control-panel {
            background: #182635;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.15);
            margin: 20px 0;
        }
        .servo-control-panel h3 {
            margin-top: 0;
            margin-bottom: 15px;
            color: #1e88e5;
        }
        .servo-toggle {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-bottom: 20px;
        }
        .servo-sliders {
            display: grid;
            grid-template-columns: 1fr;
            gap: 20px;
            margin-bottom: 15px;
        }
        .servo-slider {
            display: flex;
            flex-direction: column;
            gap: 10px;
        }
        .slider-container {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .slider-container input {
            flex-grow: 1;
        }
        .slider-value {
            min-width: 50px;
            text-align: center;
            font-weight: bold;
            color: #1e88e5;
        }
        .center-button {
            text-align: center;
            margin-top: 15px;
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
            background-color: #455a74;
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
            background-color: #1e88e5;
        }
        input:focus + .slider {
            box-shadow: 0 0 1px #1e88e5;
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
            <h1>SecurEye Pro Security Dashboard</h1>
            <button class="logout" onclick="logout()">Logout</button>
        </div>

        <div class="status-bar">
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
                Recording: <span id="recordingStatus" style="color: #43a047;">OFF</span>
            </div>
            <div class="status-item">
                Servo Position: <span id="servoPositionDisplay">90°</span>
            </div>
        </div>

        <div class="video-container">
            <img id="stream" src="" alt="Loading camera stream...">
        </div>

        <div class="controls">
            <button onclick="toggleDetection()">Toggle Face Detection</button>
            <button onclick="toggleRecognition()">Toggle Face Recognition</button>
            <button id="captureBtn" onclick="capturePhoto()">Capture Photo</button>
            <button id="recordBtn" onclick="toggleRecording()" style="background-color: #43a047;">Start Recording</button>
            <button onclick="openSettings()">Camera Settings</button>
            <button id="reportBtn" onclick="generateReport()">Generate System Report</button>
            <button id="flashBtn" onclick="toggleFlash()">Turn Flash On</button>
            <button id="stopAlarmBtn" onclick="stopAlarm()" style="background-color: #e53935;">Stop Alarm</button>
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

            <div class="servo-sliders">
                <div class="servo-slider">
                    <label for="panSlider">Pan Position (Left/Right):</label>
                    <div class="slider-container">
                        <span>0°</span>
                        <input type="range" id="panSlider" min="0" max="180" value="90" step="1" oninput="updatePanPosition(this.value)">
                        <span>180°</span>
                        <span class="slider-value" id="panValue">90°</span>
                    </div>
                </div>
                
                <div class="servo-slider">
                    <label for="tiltSlider">Tilt Position (Up/Down):</label>
                    <div class="slider-container">
                        <span>0°</span>
                        <input type="range" id="tiltSlider" min="0" max="180" value="90" step="1" oninput="updateTiltPosition(this.value)">
                        <span>180°</span>
                        <span class="slider-value" id="tiltValue">90°</span>
                    </div>
                </div>
            </div>
            
            <div class="center-button">
                <button onclick="centerServo()">Center Camera</button>
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
    </div>

    <script>
        var baseHost = document.location.origin;
        var streamUrl = baseHost + ':81/stream';
        var detection = false;
        var recognition = false;
        var autoTracking = true;
        var panPosition = 90; // Initial pan position
        var tiltPosition = 90; // Initial tilt position

        document.getElementById('stream').src = streamUrl;
        document.getElementById('panSlider').value = panPosition;
        document.getElementById('tiltSlider').value = tiltPosition;
        document.getElementById('panValue').textContent = panPosition + '°';
        document.getElementById('tiltValue').textContent = tiltPosition + '°';

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
            // Show loading indicator
            document.getElementById('captureBtn').textContent = 'Capturing...';
            document.getElementById('captureBtn').disabled = true;
            
            // Fetch the image from dashboard endpoint with capture action
            fetch(baseHost + '/dashboard?action=capture')
                .then(response => response.blob())
                .then(blob => {
                    // Create a download link
                    const url = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
                    a.style.display = 'none';
                    a.href = url;
                    a.download = 'secureyepro-capture-' + timestamp + '.jpg';
                    
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

        function generateReport() {
            // Show loading indicator
            document.getElementById('reportBtn').textContent = 'Generating...';
            document.getElementById('reportBtn').disabled = true;
            
            // Open the report in a new tab directly
            const reportUrl = baseHost + '/dashboard?action=report';
            window.open(reportUrl, '_blank');
            
            // Reset button state after a short delay
            setTimeout(() => {
                document.getElementById('reportBtn').textContent = 'Generate System Report';
                document.getElementById('reportBtn').disabled = false;
            }, 1000);
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
            fetch(baseHost + '/dashboard?action=status')
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
                    
                    // Update recording status if available
                    if (data.is_recording !== undefined) {
                        const recordStatus = document.getElementById('recordingStatus');
                        if (data.is_recording) {
                            recordStatus.textContent = 'ON';
                            recordStatus.style.color = '#e53935';
                            document.getElementById('recordBtn').textContent = 'Stop Recording';
                            document.getElementById('recordBtn').style.backgroundColor = '#e53935';
                            isRecording = true;
                        } else {
                            recordStatus.textContent = 'OFF';
                            recordStatus.style.color = '#43a047';
                            document.getElementById('recordBtn').textContent = 'Start Recording';
                            document.getElementById('recordBtn').style.backgroundColor = '#43a047';
                            isRecording = false;
                        }
                    }
                    
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

        // Servo control functions
        function toggleAutoTracking() {
            autoTracking = document.getElementById('autoTrackingToggle').checked;
            fetch(baseHost + '/control?var=servo_auto_track&val=' + (autoTracking ? '1' : '0'))
                .then(response => {
                    console.log('Auto tracking set to: ' + autoTracking);
                })
                .catch(error => console.error('Error:', error));
        }

        // Updated servo control with fine adjustment via sliders
        function updatePanPosition(value) {
            panPosition = parseInt(value);
            document.getElementById('panValue').textContent = panPosition + '°';
            sendServoPositionUpdate();
        }

        function updateTiltPosition(value) {
            tiltPosition = parseInt(value);
            document.getElementById('tiltValue').textContent = tiltPosition + '°';
            sendServoPositionUpdate();
        }

        // Debounce function to avoid too many servo updates
        let servoUpdateTimeout = null;
        function sendServoPositionUpdate() {
            if (servoUpdateTimeout !== null) {
                clearTimeout(servoUpdateTimeout);
            }
            
            servoUpdateTimeout = setTimeout(() => {
                fetch(baseHost + '/control?var=servo_position&val=' + panPosition + ',' + tiltPosition)
                    .then(response => response.json())
                    .then(data => {
                        if (data.pan_pos !== undefined && data.tilt_pos !== undefined) {
                            // Update display values if the server returns different values
                            if (data.pan_pos !== panPosition) {
                                panPosition = data.pan_pos;
                                document.getElementById('panSlider').value = panPosition;
                                document.getElementById('panValue').textContent = panPosition + '°';
                            }
                            
                            if (data.tilt_pos !== tiltPosition) {
                                tiltPosition = data.tilt_pos;
                                document.getElementById('tiltSlider').value = tiltPosition;
                                document.getElementById('tiltValue').textContent = tiltPosition + '°';
                            }
                        }
                    })
                    .catch(error => console.error('Error:', error));
                
                servoUpdateTimeout = null;
            }, 100); // 100ms debounce time
        }

        function centerServo() {
            fetch(baseHost + '/control?var=servo_center&val=1')
                .then(response => response.json())
                .then(data => {
                    if (data.pan_pos !== undefined && data.tilt_pos !== undefined) {
                        // Update sliders and displayed values
                        panPosition = data.pan_pos;
                        tiltPosition = data.tilt_pos;
                        
                        document.getElementById('panSlider').value = panPosition;
                        document.getElementById('tiltSlider').value = tiltPosition;
                        
                        document.getElementById('panValue').textContent = panPosition + '°';
                        document.getElementById('tiltValue').textContent = tiltPosition + '°';
                    }
                })
                .catch(error => console.error('Error:', error));
        }
        
        // Flash control functionality
        let flashOn = false;
        function toggleFlash() {
            flashOn = !flashOn;
            fetch(baseHost + '/control?var=flash&val=' + (flashOn ? '1' : '0'))
                .then(response => {
                    document.getElementById('flashBtn').textContent = flashOn ? 'Turn Flash Off' : 'Turn Flash On';
                })
                .catch(error => console.error('Error:', error));
        }

        // Video recording functionality
        let isRecording = false;
        let videoSocket = null;
        
        function toggleRecording() {
            if (isRecording) {
                // Stop recording
                fetch(baseHost + '/control?var=record&val=0')
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            isRecording = false;
                            document.getElementById('recordBtn').textContent = 'Start Recording';
                            document.getElementById('recordBtn').style.backgroundColor = '#43a047';
                            if (videoSocket) {
                                videoSocket.close();
                                videoSocket = null;
                            }
                            showMessage('Recording stopped');
                        } else {
                            showMessage('Failed to stop recording: ' + data.message);
                        }
                    })
                    .catch(error => {
                        console.error('Error stopping recording:', error);
                        showMessage('Error stopping recording');
                    });
            } else {
                // Start recording
                fetch(baseHost + '/control?var=record&val=1')
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            isRecording = true;
                            document.getElementById('recordBtn').textContent = 'Stop Recording';
                            document.getElementById('recordBtn').style.backgroundColor = '#e53935';
                            
                            // Start WebSocket connection for streaming
                            connectVideoWebSocket();
                            showMessage('Recording started - Streaming to connected client');
                        } else {
                            showMessage('Failed to start recording: ' + data.message);
                        }
                    })
                    .catch(error => {
                        console.error('Error starting recording:', error);
                        showMessage('Error starting recording');
                    });
            }
        }
        
        function connectVideoWebSocket() {
            // Close existing connection if any
            if (videoSocket) {
                videoSocket.close();
            }
            
            // Create WebSocket connection for video streaming
            const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = wsProtocol + '//' + window.location.host + '/ws_video';
            
            videoSocket = new WebSocket(wsUrl);
            
            videoSocket.onopen = function() {
                console.log('Video WebSocket connection established');
            };
            
            videoSocket.onclose = function() {
                console.log('Video WebSocket connection closed');
                // If we're still recording when the socket closes unexpectedly, try to reconnect
                if (isRecording) {
                    setTimeout(connectVideoWebSocket, 2000);
                }
            };
            
            videoSocket.onerror = function(error) {
                console.error('WebSocket error:', error);
            };
        }
        
        // Helper function to show status messages
        function showMessage(message) {
            const timestamp = new Date().toLocaleTimeString();
            const activityLog = document.querySelector('.activity-log');
            if (activityLog) {
                const messageItem = document.createElement('div');
                messageItem.className = 'activity-item';
                messageItem.textContent = `[${timestamp}] ${message}`;
                activityLog.insertBefore(messageItem, activityLog.firstChild);
            } else {
                console.log(message);
            }
        }

        // Function to stop the alarm buzzer
        function stopAlarm() {
            document.getElementById('stopAlarmBtn').textContent = 'Stopping...';
            document.getElementById('stopAlarmBtn').disabled = true;
            
            fetch('https://api-4u7e.onrender.com/stop_buzzer')
                .then(response => {
                    console.log('Alarm stopped successfully');
                    document.getElementById('stopAlarmBtn').textContent = 'Stop Alarm';
                    document.getElementById('stopAlarmBtn').disabled = false;
                })
                .catch(error => {
                    console.error('Error stopping alarm:', error);
                    document.getElementById('stopAlarmBtn').textContent = 'Stop Alarm';
                    document.getElementById('stopAlarmBtn').disabled = false;
                });
        }
    </script>
</body>
</html>
)rawliteral";