#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "PTT";
const char* password = "123456789";

// Pin Definitions
#define DHT_PIN 14
#define DHT_TYPE DHT22
#define PIR_PIN 23
#define LED_TOUCH_PIN 13  // Capacitive touch for light control
#define FAN_TOUCH_PIN 27  // Capacitive touch for fan mode
#define FAN_UP_PIN 26     // Capacitive touch for fan speed up
#define FAN_DOWN_PIN  34  // Capacitive touch for fan speed down

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

const int segmentPins[7] = {15, 2, 4, 5, 18, 19, 21};  // a to g segments

// Digit patterns for 0 to 4 (abcdefg)
const byte digits[5][7] = {
  {1, 1, 1, 1, 1, 1, 0},  // 0
  {0, 1, 1, 0, 0, 0, 0},  // 1
  {1, 1, 0, 1, 1, 0, 1},  // 2
  {1, 1, 1, 1, 0, 0, 1},  // 3
  {0, 1, 1, 0, 0, 1, 1}   // 4
};

// System modes
enum LightMode {OFF, AUTO, FORCE_ON};
enum FanMode {FAN_OFF, FAN_AUTO, FAN_MANUAL};

// Global variables
LightMode lightMode = OFF;
FanMode fanMode = FAN_OFF;
int fanSpeed = 0;  // 0-4 (0%, 25%, 50%, 75%, 100%)
bool currentMotionState = false;

// Timing controls
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 1000;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 2000;

// Improved debounce variables
unsigned long lastLightTouchTime = 0;
unsigned long lastFanTouchTime = 0;
unsigned long lastUpTouchTime = 0;
unsigned long lastDownTouchTime = 0;
const unsigned long DEBOUNCE_DELAY = 350;  // Increased debounce delay


// Improved motion detection variables
unsigned long lastMotionChange = 0;
const unsigned long MOTION_COOLDOWN = 500;   // Time between motion checks (1 second)
const unsigned long MOTION_TIMEOUT = 60000;   // Time after last motion to turn off lights (1 minute)
unsigned long lastMotionDetectedTime = 0;

void setup() {
    Serial.begin(115200);
    
    // Initialize pins
    pinMode(PIR_PIN, INPUT);
    pinMode(LED_TOUCH_PIN, INPUT);
    pinMode(FAN_TOUCH_PIN, INPUT);
    pinMode(FAN_UP_PIN, INPUT);
    pinMode(FAN_DOWN_PIN, INPUT);
    
    // Initialize DHT sensor
    dht.begin();
    
    // Initialize 7-segment display
    for (int i = 0; i < 7; i++) {
        pinMode(segmentPins[i], OUTPUT);
        digitalWrite(segmentPins[i], LOW);
    }
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Setup web server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/light/mode", HTTP_POST, handleLightMode);
    server.on("/api/fan/mode", HTTP_POST, handleFanMode);
    server.on("/api/fan/speed", HTTP_POST, handleFanSpeed);

    server.begin();
    Serial.println("HTTP server started");
    updateDisplay(); // Initialize display with current state
}

void loop() {
    server.handleClient();
    unsigned long currentMillis = millis();
    
    // 1. Handle Motion Detection
    handleMotionDetection(currentMillis);
    
    // 2. Process Touch Inputs
    processTouchInputs(currentMillis);
    
    // 3. Read Sensors
    if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
        lastSensorRead = currentMillis;
        readSensors();
    }
    
    // 4. Update Display and Status
    if (currentMillis - lastDisplayUpdate >= DISPLAY_INTERVAL) {
        lastDisplayUpdate = currentMillis;
        updateSystemStatus();
    }
    
    // 5. Control Light Output
    controlLightOutput();
}

void handleRoot() {
  // Serve the HTML page (unchanged)
  String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Switch Control</title>
    <style>
        * {
            box-sizing: border-box;
            font-family: Arial, sans-serif;
        }
        body {
            margin: 0;
            padding: 20px;
            background-color: #f0f2f5;
            color: #333;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: #fff;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        h1 {
            text-align: center;
            color: #2c3e50;
            margin-bottom: 30px;
        }
        .status-panel {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 15px;
            margin-bottom: 30px;
        }
        .card {
            background: #f8f9fa;
            border-radius: 8px;
            padding: 15px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.05);
        }
        .card h3 {
            margin-top: 0;
            color: #3498db;
            border-bottom: 1px solid #e1e4e8;
            padding-bottom: 10px;
        }
        .sensors p {
            display: flex;
            justify-content: space-between;
            margin: 5px 0;
        }
        .control-panel {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 20px;
        }
        .control-card {
            background: linear-gradient(145deg, #e6e9ef, #ffffff);
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.07);
            transition: all 0.3s ease;
        }
        .control-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 6px 12px rgba(0,0,0,0.1);
        }
        .control-card h3 {
            text-align: center;
            margin-top: 0;
            color: #2c3e50;
        }
        .btn-group {
            display: flex;
            justify-content: space-between;
            margin: 15px 0;
        }
        button {
            padding: 10px 15px;
            border: none;
            border-radius: 5px;
            background-color: #3498db;
            color: white;
            cursor: pointer;
            transition: background-color 0.3s;
        }
        button:hover {
            background-color: #2980b9;
        }
        button:active {
            background-color: #1f6dad;
        }
        button.active {
            background-color: #27ae60;
        }
        button.disabled {
            background-color: #95a5a6;
            cursor: not-allowed;
        }
        .fan-speed {
            text-align: center;
            padding: 10px 0;
        }
        .fan-speed-controls {
            display: flex;
            justify-content: center;
            gap: 10px;
            margin-top: 15px;
        }
        .fan-speed-value {
            font-size: 24px;
            font-weight: bold;
            color: #2c3e50;
            margin: 10px 0;
        }
        .speed-indicator {
            display: flex;
            justify-content: space-between;
            margin: 10px 0;
        }
        .speed-dot {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background-color: #e0e0e0;
        }
        .speed-dot.active {
            background-color: #3498db;
        }
        .fa-circle {
            display: inline-block;
            width: 10px;
            height: 10px;
            border-radius: 50%;
            margin-right: 5px;
        }
        .status-active {
            background-color: #2ecc71;
        }
        .status-inactive {
            background-color: #e74c3c;
        }
        @media (max-width: 600px) {
            .status-panel, .control-panel {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Smart Switch Control Panel</h1>
        
        <div class="status-panel">
            <div class="card sensors">
                <h3>Environment</h3>
                <p>Temperature: <span id="temperature">--</span>°C</p>
                <p>Humidity: <span id="humidity">--</span>%</p>
                <p>Heat Index: <span id="heat-index">--</span>°C</p>
                <p>Motion: <span id="motion-indicator">--</span></p>
            </div>
            
            <div class="card status">
                <h3>System Status</h3>
                <p>Light Mode: <span id="light-mode">--</span></p>
                <p>Light State: <span id="light-state">--</span></p>
                <p>Fan Mode: <span id="fan-mode">--</span></p>
                <p>Fan Speed: <span id="fan-speed">--</span>%</p>
            </div>
        </div>
        
        <div class="control-panel">
            <div class="control-card">
                <h3>Light Control</h3>
                <div class="btn-group">
                    <button id="light-off" onclick="setLightMode('OFF')">OFF</button>
                    <button id="light-auto" onclick="setLightMode('AUTO')">AUTO</button>
                    <button id="light-on" onclick="setLightMode('ON')">ON</button>
                </div>
            </div>
            
            <div class="control-card">
                <h3>Fan Control</h3>
                <div class="btn-group">
                    <button id="fan-off" onclick="setFanMode('OFF')">OFF</button>
                    <button id="fan-auto" onclick="setFanMode('AUTO')">AUTO</button>
                    <button id="fan-manual" onclick="setFanMode('MANUAL')">MANUAL</button>
                </div>
                <div class="fan-speed">
                    <p>Fan Speed Level</p>
                    <div class="speed-indicator">
                        <div class="speed-dot" id="speed-dot-0"></div>
                        <div class="speed-dot" id="speed-dot-1"></div>
                        <div class="speed-dot" id="speed-dot-2"></div>
                        <div class="speed-dot" id="speed-dot-3"></div>
                        <div class="speed-dot" id="speed-dot-4"></div>
                    </div>
                    <p class="fan-speed-value" id="fan-speed-value">0%</p>
                    <div class="fan-speed-controls">
                        <button onclick="changeFanSpeed(-1)">−</button>
                        <button onclick="changeFanSpeed(1)">+</button>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script>
        // Speed levels corresponding to slider positions
        const speedLevels = [0, 25, 50, 75, 100];
        let currentStatus = {
            speedLevel: 0,
            fanMode: 'OFF',
            fanSpeed: 0
        };

        // Initial status load
        window.onload = function() {
            fetchStatus();
            // Update status every 2 seconds
            setInterval(fetchStatus, 2000);
        };

        // Update UI based on current status
        function updateUI(data) {
            // Store current status
            currentStatus = data;
            
            // Update environmental data
            if(data.temperature !== undefined) {
                document.getElementById('temperature').textContent = data.temperature.toFixed(1);
                document.getElementById('humidity').textContent = data.humidity.toFixed(1);
                document.getElementById('heat-index').textContent = data.heatIndex.toFixed(1);
            }
            
            // Update motion indicator
            const motionIndicator = document.getElementById('motion-indicator');
            if (data.motion) {
                motionIndicator.innerHTML = '<span class="fa-circle status-active"></span> Detected';
            } else {
                motionIndicator.innerHTML = '<span class="fa-circle status-inactive"></span> None';
            }
            
            // Update system status
            document.getElementById('light-mode').textContent = data.lightMode;
            document.getElementById('light-state').textContent = data.lightState ? 'ON' : 'OFF';
            document.getElementById('fan-mode').textContent = data.fanMode;
            document.getElementById('fan-speed').textContent = speedLevels[data.speedLevel];
            
            // Update active buttons
            updateActiveButton('light', data.lightMode);
            updateActiveButton('fan', data.fanMode);
            
            // Update fan speed display
            document.getElementById('fan-speed-value').textContent = ${speedLevels[data.speedLevel]}%;
            
            // Update speed indicator dots
            for (let i = 0; i < 5; i++) {
                const dot = document.getElementById(speed-dot-${i});
                if (i <= data.speedLevel) {
                    dot.classList.add('active');
                } else {
                    dot.classList.remove('active');
                }
            }
            
            // Disable controls if not in manual mode
            const speedButtons = document.querySelectorAll('.fan-speed-controls button');
            if (data.fanMode === 'MANUAL') {
                speedButtons.forEach(btn => btn.disabled = false);
            } else {
                speedButtons.forEach(btn => btn.disabled = true);
            }
        }

        // Mark active mode button
        function updateActiveButton(device, mode) {
            const buttons = ['off', 'auto', device === 'light' ? 'on' : 'manual'];
            buttons.forEach(btn => {
                const button = document.getElementById(${device}-${btn});
                if (mode.toLowerCase() === btn) {
                    button.classList.add('active');
                } else {
                    button.classList.remove('active');
                }
            });
        }

        // Change fan speed by increment/decrement
        function changeFanSpeed(change) {
            const newLevel = Math.max(0, Math.min(4, currentStatus.speedLevel + change));
            setFanSpeed(newLevel);
        }

        // API calls
        async function fetchStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                updateUI(data);
            } catch (error) {
                console.error('Error fetching status:', error);
            }
        }

        async function setLightMode(mode) {
            try {
                const response = await fetch('/api/light/mode', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: mode=${mode}
                });
                if (response.ok) {
                    fetchStatus();
                }
            } catch (error) {
                console.error('Error setting light mode:', error);
            }
        }

        async function setFanMode(mode) {
            try {
                const response = await fetch('/api/fan/mode', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: mode=${mode}
                });
                if (response.ok) {
                    fetchStatus();
                }
            } catch (error) {
                console.error('Error setting fan mode:', error);
            }
        }

        async function setFanSpeed(level) {
            if (currentStatus.fanMode !== 'MANUAL') {
                alert('Fan speed can only be adjusted in MANUAL mode');
                return;
            }
            
            try {
                const response = await fetch('/api/fan/speed', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: level=${level}
                });
                if (response.ok) {
                    fetchStatus();
                }
            } catch (error) {
                console.error('Error setting fan speed:', error);
            }
        }
    </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

void handleStatus() {
  DynamicJsonDocument doc(256);
  
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  doc["temperature"] = isnan(temperature) ? 0.0 : temperature;
  doc["humidity"] = isnan(humidity) ? 0.0 : humidity;
  doc["heatIndex"] = computeHeatIndex(doc["temperature"], doc["humidity"]);
  doc["motion"] = currentMotionState;
  doc["lightMode"] = getLightModeString();
  doc["lightState"] = (lightMode == FORCE_ON || (lightMode == AUTO && currentMotionState));
  doc["fanMode"] = getFanModeString();
  doc["fanSpeed"] = fanSpeed * 25;
  doc["speedLevel"] = fanSpeed;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleLightMode() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String mode = server.arg("mode");
  if (mode == "OFF") {
    lightMode = OFF;
    server.send(200, "text/plain", "OK");
  } else if (mode == "AUTO") {
    lightMode = AUTO;
    server.send(200, "text/plain", "OK");
  } else if (mode == "ON") {
    lightMode = FORCE_ON;
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Invalid mode");
  }
}

void handleFanMode() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String mode = server.arg("mode");
  if (mode == "OFF") {
    fanMode = FAN_OFF;
    fanSpeed = 0;
    updateDisplay();
    server.send(200, "text/plain", "OK");
  } else if (mode == "AUTO") {
    fanMode = FAN_AUTO;
    server.send(200, "text/plain", "OK");
  } else if (mode == "MANUAL") {
    fanMode = FAN_MANUAL;
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Invalid mode");
  }
}

void handleFanSpeed() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  if (fanMode != FAN_MANUAL) {
    server.send(400, "text/plain", "Fan not in manual mode");
    return;
  }
  
  int level = server.arg("level").toInt();
  if (level >= 0 && level <= 4) {
    fanSpeed = level;
    updateDisplay();
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Invalid level");
  }
}

String getLightModeString() {
    switch(lightMode) {
        case OFF: return "OFF";
        case AUTO: return "AUTO";
        case FORCE_ON: return "ON";
        default: return "UNKNOWN";
    }
}

String getFanModeString() {
    switch(fanMode) {
        case FAN_OFF: return "OFF";
        case FAN_AUTO: return "AUTO";
        case FAN_MANUAL: return "MANUAL";
        default: return "UNKNOWN";
    }
}

void handleMotionDetection(unsigned long currentMillis) {
    bool motionReading = digitalRead(PIR_PIN);
    
    // If motion is detected, update the last detected time
    if (motionReading == HIGH) {
        lastMotionDetectedTime = currentMillis;
        if (!currentMotionState) {
            currentMotionState = true;
            Serial.println("Motion detected!");
        }
    }
    // Only consider no motion if we haven't seen any for the timeout period
    else if (currentMotionState && (currentMillis - lastMotionDetectedTime > MOTION_TIMEOUT)) {
        currentMotionState = false;
        Serial.println("No motion detected for timeout period");
    }
    
    // Small delay between sensor reads to prevent bouncing
    if (currentMillis - lastMotionChange > MOTION_COOLDOWN) {
        lastMotionChange = currentMillis;
    }
}

void processTouchInputs(unsigned long currentMillis) {
    // Light mode touch
    if (digitalRead(LED_TOUCH_PIN)) {
        if (currentMillis - lastLightTouchTime > DEBOUNCE_DELAY) {
            lastLightTouchTime = currentMillis;
            lightMode = (LightMode)((lightMode + 1) % 3);
            Serial.print("Light mode changed to: ");
            Serial.println(getLightModeString());
        }
    }
    
    // Fan mode touch
    if (digitalRead(FAN_TOUCH_PIN)) {
        if (currentMillis - lastFanTouchTime > DEBOUNCE_DELAY) {
            lastFanTouchTime = currentMillis;
            fanMode = (FanMode)((fanMode + 1) % 3);
            Serial.print("Fan mode changed to: ");
            Serial.println(getFanModeString());
            updateDisplay();
        }
    }
    
    // Manual fan control
    if (fanMode == FAN_MANUAL) {
        // Fan speed up
        if (digitalRead(FAN_UP_PIN)) {
            if (currentMillis - lastUpTouchTime > DEBOUNCE_DELAY) {
                lastUpTouchTime = currentMillis;
                fanSpeed = min(fanSpeed + 1, 4);
                updateDisplay();
                Serial.print("Fan speed increased to: ");
                Serial.print(fanSpeed * 25);
                Serial.println("%");
            }
        }
        
        // Fan speed down
        if (digitalRead(FAN_DOWN_PIN)) {
            if (currentMillis - lastDownTouchTime > DEBOUNCE_DELAY) {
                lastDownTouchTime = currentMillis;
                fanSpeed = max(fanSpeed - 1, 0);
                updateDisplay();
                Serial.print("Fan speed decreased to: ");
                Serial.print(fanSpeed * 25);
                Serial.println("%");
            }
        }
    }
}

void readSensors() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    if (!isnan(temperature) && !isnan(humidity)) {
        // Auto fan control
        if (fanMode == FAN_AUTO && currentMotionState == HIGH) {
            float feels_like = computeHeatIndex(temperature, humidity);
            int newFanSpeed = decisionTreeFanSpeed(feels_like, humidity) / 25;
            
            if (newFanSpeed != fanSpeed) {
                fanSpeed = newFanSpeed;
                updateDisplay();
                Serial.print("Auto-adjusted fan speed to: ");
                Serial.print(fanSpeed * 25);
                Serial.println("%");
            }
        }
    }
}

void updateSystemStatus() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    Serial.println("\n=== System Status ===");
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println("°C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");
    
    if (!isnan(temperature) && !isnan(humidity)) {
        Serial.print("Feels Like: "); 
        Serial.print(computeHeatIndex(temperature, humidity)); 
        Serial.println("°C");
    }
    
    Serial.print("Light Mode: "); Serial.println(getLightModeString());
    Serial.print("Light State: "); Serial.println((lightMode == FORCE_ON || (lightMode == AUTO && currentMotionState)) ? "ON" : "OFF");
    Serial.print("Fan Mode: "); Serial.println(getFanModeString());
    Serial.print("Fan Speed: "); Serial.print(fanSpeed * 25); Serial.println("%");
    Serial.print("Motion: "); Serial.println(currentMotionState ? "DETECTED" : "NONE");
    Serial.println("====================");
}

void controlLightOutput() {
    static bool currentLightState = false;
    bool newLightState = (lightMode == FORCE_ON) || (lightMode == AUTO && currentMotionState == HIGH);
    
    if (newLightState != currentLightState) {
        currentLightState = newLightState;
        digitalWrite(LED_TOUCH_PIN, currentLightState ? HIGH : LOW);
        Serial.print("Light turned ");
        Serial.println(currentLightState ? "ON" : "OFF");
    }
}

// Helper function to update the 7-segment display
void updateDisplay() {
    int displayValue;
    if (fanMode == FAN_MANUAL) {
        displayValue = fanSpeed; // Show actual speed level in manual mode
    } else if (fanMode == FAN_AUTO && fanSpeed > 0) {
        displayValue = fanSpeed; // Show current speed in auto mode if fan is on
    } else {
        displayValue = 0; // Show 0 in off mode or when fan is off in auto mode
    }
    
    // Make sure we don't exceed our digit patterns array
    displayValue = constrain(displayValue, 0, 4);
    
    // Update the display
    for (int i = 0; i < 7; i++) {
        digitalWrite(segmentPins[i], digits[displayValue][i]);
    }
}

// Helper functions
float computeHeatIndex(float temp, float hum) {
    return -8.784695 + 1.61139411 * temp + 2.338549 * hum +
           -0.14611605 * temp * hum + -0.012308094 * (temp * temp) +
           -0.01642482 * (hum * hum) + 0.002211732 * (temp * temp * hum) +
           0.00072546 * (temp * hum * hum) - 0.000003582 * (temp * temp * hum * hum);
}

int decisionTreeFanSpeed(float feels_like, float hum) {
    if (feels_like < 20) return 0;
    else if (feels_like < 25) return (hum < 50) ? 25 : 50;
    else if (feels_like < 30) return (hum < 70) ? 50 : 75;
    else if (feels_like < 35) return 75;
    else return 100;
}

int fanSpeedToDigit(int fan_speed) {
    // Ensure we return a valid digit (0-4)
    return constrain(fan_speed, 0, 4);
}

void displayDigit(int digit) {
    digit = constrain(digit, 0, 4); // Safety check
    for (int i = 0; i < 7; i++) {
        digitalWrite(segmentPins[i], digits[digit][i]);
    }
}