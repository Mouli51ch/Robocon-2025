#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ==== Wi-Fi credentials ====
const char* ssid = "OHIO";
const char* password = "whoami123";

// ==== Motor driver pins (example for L298N) ====
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25
#define ENA 32 // Motor A speed pin
#define ENB 33 // Motor B speed pin

// ==== PWM range for analogWrite ====
#define MAX_SPEED 255

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

int joyX = 0, joyY = 0;  // Joystick values (-100 to 100)
bool emergencyStop = false;
float accelerationLevel = 50.0; // 0-100 fine control

// ==== HTML page ====
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Robot Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<style>
* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

body { 
  font-family: 'Arial', sans-serif;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  height: 100vh;
  overflow: hidden;
  touch-action: none;
}

.container {
  display: flex;
  flex-direction: column;
  height: 100vh;
  padding: 10px;
}

.header {
  text-align: center;
  padding: 10px 0;
}

.header h2 {
  font-size: 1.5rem;
  margin-bottom: 5px;
}

.status {
  font-size: 0.9rem;
  opacity: 0.8;
}

.controls {
  flex: 1;
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 20px;
  max-width: 800px;
  margin: 0 auto;
  width: 100%;
}

.left-panel, .right-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

/* Joystick Styles */
#joystick {
  width: 200px;
  height: 200px;
  border-radius: 50%;
  background: rgba(255,255,255,0.2);
  border: 3px solid rgba(255,255,255,0.3);
  position: relative;
  margin-bottom: 20px;
  box-shadow: 0 8px 32px rgba(0,0,0,0.3);
}

#stick {
  width: 60px;
  height: 60px;
  border-radius: 50%;
  background: rgba(255,255,255,0.9);
  position: absolute;
  top: 70px;
  left: 70px;
  box-shadow: 0 4px 16px rgba(0,0,0,0.3);
  transition: all 0.1s ease;
}

.joystick-label {
  font-size: 0.9rem;
  opacity: 0.8;
}

/* Directional Keys */
.dpad-container {
  position: relative;
  width: 200px;
  height: 200px;
  margin-bottom: 20px;
}

.dpad-btn {
  position: absolute;
  width: 60px;
  height: 60px;
  background: rgba(255,255,255,0.2);
  border: 2px solid rgba(255,255,255,0.3);
  border-radius: 10px;
  color: white;
  font-size: 1.2rem;
  font-weight: bold;
  cursor: pointer;
  user-select: none;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.1s ease;
  box-shadow: 0 4px 16px rgba(0,0,0,0.2);
}

.dpad-btn:active, .dpad-btn.pressed {
  background: rgba(255,255,255,0.6);
  transform: scale(0.95);
  border-color: #2ecc71;
}

#up-btn { top: 0; left: 70px; }
#down-btn { bottom: 0; left: 70px; }
#left-btn { top: 70px; left: 0; }
#right-btn { top: 70px; right: 0; }

/* Control Buttons */
.control-buttons {
  display: flex;
  flex-direction: column;
  gap: 15px;
  width: 100%;
  max-width: 300px;
}

.btn {
  padding: 15px 20px;
  border: none;
  border-radius: 10px;
  font-size: 1rem;
  font-weight: bold;
  cursor: pointer;
  transition: all 0.1s ease;
  box-shadow: 0 4px 16px rgba(0,0,0,0.2);
}

.emergency-btn {
  background: #ff4757;
  color: white;
  font-size: 1.1rem;
}

.emergency-btn:active {
  background: #ff3838;
  transform: scale(0.95);
}

/* Acceleration Slider */
.acceleration-control {
  background: rgba(255,255,255,0.1);
  border: 2px solid rgba(255,255,255,0.2);
  border-radius: 15px;
  padding: 20px;
  text-align: center;
}

.acceleration-label {
  font-size: 1rem;
  margin-bottom: 10px;
  font-weight: bold;
}

.acceleration-value {
  font-size: 1.5rem;
  color: #2ecc71;
  margin-bottom: 15px;
  text-shadow: 0 0 10px rgba(46, 204, 113, 0.5);
}

.slider {
  width: 100%;
  height: 8px;
  border-radius: 5px;
  background: rgba(255,255,255,0.3);
  outline: none;
  opacity: 0.8;
  transition: opacity 0.2s;
  -webkit-appearance: none;
  appearance: none;
}

.slider:hover {
  opacity: 1;
}

.slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 25px;
  height: 25px;
  border-radius: 50%;
  background: #2ecc71;
  cursor: pointer;
  box-shadow: 0 0 10px rgba(46, 204, 113, 0.5);
}

.slider::-moz-range-thumb {
  width: 25px;
  height: 25px;
  border-radius: 50%;
  background: #2ecc71;
  cursor: pointer;
  border: none;
  box-shadow: 0 0 10px rgba(46, 204, 113, 0.5);
}

/* Connection Status */
.connection-status {
  position: fixed;
  top: 10px;
  right: 10px;
  padding: 8px 15px;
  border-radius: 20px;
  font-size: 0.8rem;
  font-weight: bold;
  z-index: 1000;
}

.connected {
  background: linear-gradient(45deg, #2ecc71, #27ae60);
  color: white;
  box-shadow: 0 4px 15px rgba(46, 204, 113, 0.3);
}

.disconnected {
  background: linear-gradient(45deg, #e74c3c, #c0392b);
  color: white;
  box-shadow: 0 4px 15px rgba(231, 76, 60, 0.3);
}

/* Performance Monitor */
.performance-monitor {
  position: fixed;
  top: 10px;
  left: 10px;
  background: rgba(0,0,0,0.8);
  padding: 10px;
  border-radius: 8px;
  font-family: monospace;
  font-size: 0.7rem;
  z-index: 1000;
}

/* Responsive Design */
@media (max-width: 768px) {
  .controls {
    flex-direction: column;
    gap: 25px;
  }
  
  .left-panel, .right-panel {
    width: 100%;
  }
  
  #joystick, .dpad-container {
    width: 180px;
    height: 180px;
  }
  
  #stick {
    width: 50px;
    height: 50px;
    top: 65px;
    left: 65px;
  }
  
  .dpad-btn {
    width: 50px;
    height: 50px;
    font-size: 1rem;
  }
  
  #up-btn, #down-btn { left: 65px; }
  #left-btn, #right-btn { top: 65px; }
  
  .performance-monitor {
    position: relative;
    margin: 10px;
  }
}

@media (max-width: 480px) {
  .header h2 {
    font-size: 1.3rem;
  }
  
  #joystick, .dpad-container {
    width: 160px;
    height: 160px;
  }
  
  #stick {
    width: 45px;
    height: 45px;
    top: 57.5px;
    left: 57.5px;
  }
  
  .dpad-btn {
    width: 45px;
    height: 45px;
    font-size: 0.9rem;
  }
  
  #up-btn, #down-btn { left: 57.5px; }
  #left-btn, #right-btn { top: 57.5px; }
}

/* Button Press Animation */
@keyframes buttonPulse {
  0% { transform: scale(0.95); }
  50% { transform: scale(0.98); }
  100% { transform: scale(0.95); }
}

.dpad-btn.pressed {
  animation: buttonPulse 0.5s infinite;
}
</style>
</head>
<body>
<div class="connection-status disconnected" id="connectionStatus">Disconnected</div>

<div class="performance-monitor" id="performanceMonitor">
  Signal: 0ms<br>
  Commands/sec: 0<br>
  Active: None
</div>

<div class="container">
  <div class="header">
    <h2>ESP32 Robot Controller</h2>
    <div class="status">Advanced Multi-touch Control System</div>
  </div>

  <div class="controls">
    <div class="left-panel">
      <div id="joystick">
        <div id="stick"></div>
      </div>
      <div class="joystick-label">Analog Control</div>
    </div>

    <div class="right-panel">
      <div class="dpad-container">
        <button class="dpad-btn" id="up-btn">↑</button>
        <button class="dpad-btn" id="left-btn">←</button>
        <button class="dpad-btn" id="right-btn">→</button>
        <button class="dpad-btn" id="down-btn">↓</button>
      </div>
      <div class="joystick-label">Digital Control</div>
    </div>
  </div>

  <div class="control-buttons">
    <div class="acceleration-control">
      <div class="acceleration-label">Power Control</div>
      <div class="acceleration-value" id="accelValue">50%</div>
      <input type="range" min="10" max="100" value="50" class="slider" id="accelSlider">
    </div>
    <button class="btn emergency-btn" onclick="emergencyStop()">
      🛑 EMERGENCY STOP
    </button>
  </div>
</div>

<script>
let ws;
let connected = false;
let accelerationLevel = 50;
let currentDirection = { x: 0, y: 0 };
let pressedButtons = new Set();
let commandInterval = null;
let signalStartTime = 0;
let commandCount = 0;
let lastSecond = Date.now();

// Performance monitoring
function updatePerformanceMonitor() {
  const now = Date.now();
  if (now - lastSecond >= 1000) {
    const activeControls = [];
    if (Math.abs(currentDirection.x) > 0 || Math.abs(currentDirection.y) > 0) {
      activeControls.push('Joystick');
    }
    if (pressedButtons.size > 0) {
      activeControls.push('D-Pad');
    }
    
    const signalDelay = signalStartTime > 0 ? now - signalStartTime : 0;
    document.getElementById('performanceMonitor').innerHTML = 
      `Signal: ${signalDelay}ms<br>Commands/sec: ${commandCount}<br>Active: ${activeControls.join(', ') || 'None'}`;
    
    commandCount = 0;
    lastSecond = now;
  }
}

// Initialize WebSocket connection
function initWebSocket() {
  ws = new WebSocket("ws://" + location.hostname + ":81/");
  
  ws.onopen = function() {
    connected = true;
    document.getElementById('connectionStatus').textContent = 'Connected';
    document.getElementById('connectionStatus').className = 'connection-status connected';
  };
  
  ws.onclose = function() {
    connected = false;
    document.getElementById('connectionStatus').textContent = 'Disconnected';
    document.getElementById('connectionStatus').className = 'connection-status disconnected';
    setTimeout(initWebSocket, 2000);
  };
  
  ws.onerror = function() {
    connected = false;
  };
}

// Send command via WebSocket
function sendCommand(x, y, immediate = false) {
  if (connected && ws) {
    signalStartTime = Date.now();
    ws.send(x + "," + y + "," + accelerationLevel.toFixed(1));
    commandCount++;
  }
}

// Continuous command sending for pressed buttons
function startContinuousCommand() {
  if (commandInterval) return;
  
  commandInterval = setInterval(() => {
    if (pressedButtons.size > 0) {
      let x = 0, y = 0;
      
      if (pressedButtons.has('up')) y = 100;
      if (pressedButtons.has('down')) y = -100;
      if (pressedButtons.has('left')) x = -100;
      if (pressedButtons.has('right')) x = 100;
      
      // Handle diagonal movement
      if (pressedButtons.has('up') && pressedButtons.has('left')) { x = -71; y = 71; }
      if (pressedButtons.has('up') && pressedButtons.has('right')) { x = 71; y = 71; }
      if (pressedButtons.has('down') && pressedButtons.has('left')) { x = -71; y = -71; }
      if (pressedButtons.has('down') && pressedButtons.has('right')) { x = 71; y = -71; }
      
      sendCommand(x, y);
    } else {
      stopContinuousCommand();
      sendCommand(0, 0);
    }
  }, 50); // Send commands every 50ms while pressed
}

function stopContinuousCommand() {
  if (commandInterval) {
    clearInterval(commandInterval);
    commandInterval = null;
  }
}

// Joystick handling
let joystick = document.getElementById("joystick");
let stick = document.getElementById("stick");
let isDragging = false;

function handleJoystickMove(clientX, clientY) {
  let rect = joystick.getBoundingClientRect();
  let centerX = rect.left + rect.width / 2;
  let centerY = rect.top + rect.height / 2;
  
  let x = clientX - centerX;
  let y = clientY - centerY;
  
  let maxRadius = rect.width / 2 - 30;
  let distance = Math.min(Math.sqrt(x*x + y*y), maxRadius);
  let angle = Math.atan2(y, x);
  
  let stickX = distance * Math.cos(angle);
  let stickY = distance * Math.sin(angle);
  
  stick.style.left = (stickX + rect.width/2 - 30) + "px";
  stick.style.top = (stickY + rect.height/2 - 30) + "px";
  
  let joyX = Math.round((stickX / maxRadius) * 100);
  let joyY = Math.round((-stickY / maxRadius) * 100);
  
  currentDirection.x = joyX;
  currentDirection.y = joyY;
  sendCommand(joyX, joyY);
}

function resetJoystick() {
  stick.style.left = "70px";
  stick.style.top = "70px";
  currentDirection.x = 0;
  currentDirection.y = 0;
  sendCommand(0, 0);
}

// Touch events for joystick
joystick.addEventListener("touchstart", function(e) {
  e.preventDefault();
  isDragging = true;
});

joystick.addEventListener("touchmove", function(e) {
  if (isDragging) {
    e.preventDefault();
    let touch = e.touches[0];
    handleJoystickMove(touch.clientX, touch.clientY);
  }
});

joystick.addEventListener("touchend", function(e) {
  e.preventDefault();
  isDragging = false;
  resetJoystick();
});

// Mouse events for joystick (desktop support)
joystick.addEventListener("mousedown", function(e) {
  e.preventDefault();
  isDragging = true;
});

document.addEventListener("mousemove", function(e) {
  if (isDragging) {
    e.preventDefault();
    handleJoystickMove(e.clientX, e.clientY);
  }
});

document.addEventListener("mouseup", function(e) {
  if (isDragging) {
    e.preventDefault();
    isDragging = false;
    resetJoystick();
  }
});

// Directional pad handling with continuous sending
let dpadButtons = {
  'up-btn': 'up',
  'down-btn': 'down',
  'left-btn': 'left',
  'right-btn': 'right'
};

Object.keys(dpadButtons).forEach(buttonId => {
  let button = document.getElementById(buttonId);
  let direction = dpadButtons[buttonId];
  
  // Touch events
  button.addEventListener("touchstart", function(e) {
    e.preventDefault();
    pressedButtons.add(direction);
    button.classList.add('pressed');
    startContinuousCommand();
  });
  
  button.addEventListener("touchend", function(e) {
    e.preventDefault();
    pressedButtons.delete(direction);
    button.classList.remove('pressed');
  });
  
  // Mouse events
  button.addEventListener("mousedown", function(e) {
    e.preventDefault();
    pressedButtons.add(direction);
    button.classList.add('pressed');
    startContinuousCommand();
  });
  
  button.addEventListener("mouseup", function(e) {
    e.preventDefault();
    pressedButtons.delete(direction);
    button.classList.remove('pressed');
  });
  
  button.addEventListener("mouseleave", function(e) {
    e.preventDefault();
    pressedButtons.delete(direction);
    button.classList.remove('pressed');
  });
});

// Acceleration slider control
document.getElementById('accelSlider').addEventListener('input', function(e) {
  accelerationLevel = parseFloat(e.target.value);
  document.getElementById('accelValue').textContent = accelerationLevel.toFixed(0) + '%';
  
  // Update slider color based on value
  const slider = e.target;
  const percent = (accelerationLevel - 10) / 90 * 100;
  slider.style.background = `linear-gradient(to right, #2ecc71 0%, #2ecc71 ${percent}%, rgba(255,255,255,0.3) ${percent}%, rgba(255,255,255,0.3) 100%)`;
});

// Emergency stop
function emergencyStop() {
  if (connected && ws) {
    ws.send("STOP");
  }
  resetJoystick();
  pressedButtons.clear();
  stopContinuousCommand();
  
  // Visual feedback
  document.querySelectorAll('.dpad-btn').forEach(btn => {
    btn.classList.remove('pressed');
  });
}

// Initialize connection and monitoring
window.addEventListener('load', function() {
  initWebSocket();
  
  // Start performance monitoring
  setInterval(updatePerformanceMonitor, 100);
  
  // Initialize slider appearance
  const slider = document.getElementById('accelSlider');
  slider.style.background = 'linear-gradient(to right, #2ecc71 0%, #2ecc71 44%, rgba(255,255,255,0.3) 44%, rgba(255,255,255,0.3) 100%)';
});

// Prevent default touch behaviors
document.addEventListener('touchmove', function(e) {
  if (e.target.closest('#joystick') || e.target.closest('.dpad-btn')) {
    e.preventDefault();
  }
}, { passive: false });

// Cleanup on page unload
window.addEventListener('beforeunload', function() {
  stopContinuousCommand();
  if (connected && ws) {
    ws.send("0,0,0");
  }
});

</script>
</body>
</html>
)rawliteral";

// ==== Motor control ====
void setMotorSpeed(int leftSpeed, int rightSpeed) {
  if (emergencyStop) { leftSpeed = 0; rightSpeed = 0; }

  // Constrain speeds to valid range
  leftSpeed = constrain(leftSpeed, -MAX_SPEED, MAX_SPEED);
  rightSpeed = constrain(rightSpeed, -MAX_SPEED, MAX_SPEED);

  // Left motor
  if (leftSpeed >= 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
  analogWrite(ENA, abs(leftSpeed));

  // Right motor
  if (rightSpeed >= 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
  analogWrite(ENB, abs(rightSpeed));
}

// ==== Joystick mapping with fine acceleration control ====
void handleJoystick(int x, int y, float accelLevel) {
  int leftMotor = y + x;
  int rightMotor = y - x;
  
  // Map to motor speed range
  leftMotor = map(leftMotor, -200, 200, -MAX_SPEED, MAX_SPEED);
  rightMotor = map(rightMotor, -200, 200, -MAX_SPEED, MAX_SPEED);
  
  // Apply fine acceleration scaling (10% to 100%)
  float accelMultiplier = accelLevel / 100.0;
  
  leftMotor = (int)(leftMotor * accelMultiplier);
  rightMotor = (int)(rightMotor * accelMultiplier);
  
  setMotorSpeed(leftMotor, rightMotor);
}

// ==== WebSocket events ====
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.printf("Client #%u connected from %s\n", num, payload);
  }
  
  if (type == WStype_DISCONNECTED) {
    Serial.printf("Client #%u disconnected\n", num);
    // Stop motors when client disconnects
    setMotorSpeed(0, 0);
  }
  
  if (type == WStype_TEXT) {
    String data = String((char*)payload);
    
    if (data == "STOP") {
      emergencyStop = true;
      setMotorSpeed(0, 0);
      Serial.println("🛑 Emergency Stop Activated!");
      return;
    }
    
    emergencyStop = false;
    
    // Parse x,y,acceleration format
    int firstComma = data.indexOf(',');
    int secondComma = data.indexOf(',', firstComma + 1);
    
    if (firstComma > 0 && secondComma > 0) {
      joyX = data.substring(0, firstComma).toInt();
      joyY = data.substring(firstComma + 1, secondComma).toInt();
      accelerationLevel = data.substring(secondComma + 1).toFloat();
      
      // Constrain acceleration level
      accelerationLevel = constrain(accelerationLevel, 10.0, 100.0);
      
      handleJoystick(joyX, joyY, accelerationLevel);
      
      // Debug output (reduced frequency to avoid spam)
      static unsigned long lastDebugTime = 0;
      if (millis() - lastDebugTime > 200) { // Debug every 200ms
        Serial.printf("X:%d, Y:%d, Power:%.1f%%\n", joyX, joyY, accelerationLevel);
        lastDebugTime = millis();
      }
    }
  }
}

// ==== Watchdog timer to stop motors if no signal ====
unsigned long lastCommandTime = 0;
const unsigned long COMMAND_TIMEOUT = 2000; // 2 seconds timeout

void checkCommandTimeout() {
  if (!emergencyStop && millis() - lastCommandTime > COMMAND_TIMEOUT) {
    setMotorSpeed(0, 0);
    Serial.println("⚠️  Command timeout - Motors stopped");
    lastCommandTime = millis(); // Reset to prevent spam
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Robot Controller Starting ===");

  // Initialize motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Ensure motors are stopped initially
  setMotorSpeed(0, 0);
  Serial.println("✓ Motor pins initialized");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to WiFi network: %s", ssid);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✓ WiFi connected successfully!");
    Serial.printf("📡 IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("🌐 Access controller at: http://%s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println();
    Serial.println("❌ WiFi connection failed!");
    return;
  }

  // Initialize web server
  server.on("/", [](){ 
    server.send_P(200, "text/html", webpage);
    Serial.println("📄 Web page served to client");
  });
  
  server.begin();
  Serial.println("✓ Web server started on port 80");

  // Initialize WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("✓ WebSocket server started on port 81");
  
  Serial.println("=== Robot Controller Ready! ===");
  Serial.printf("Total clients can connect: %d\n", WEBSOCKETS_SERVER_CLIENT_MAX);
  
  lastCommandTime = millis();
}

void loop() {
  server.handleClient();
  webSocket.loop();
  
  // Update last command time when receiving valid commands
  if (joyX != 0 || joyY != 0) {
    lastCommandTime = millis();
  }
  
  // Check for command timeout
  checkCommandTimeout();
  
  // Small delay to prevent overwhelming the system
  delay(5);
}