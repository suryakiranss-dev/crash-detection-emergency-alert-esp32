
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS++.h>
#include <time.h>
#include <MAX30105.h>
#include "heartRate.h"
#include <Firebase_ESP_Client.h>

// Real WiFi + Firebase credentials live in secrets.h (git-ignored).
// Copy secrets_template.h -> secrets.h and fill in your own values.
#include "secrets.h"

#define STATUS_LED 2

// ---------------- Pin Definitions ----------------
#define ENA 25
#define IN1 27
#define IN2 26
#define ENB 33
#define IN3 14
#define IN4 32

#define MQ2_PIN 34
#define FLAME_PIN 4
#define VIB_PIN 35

#define ACCIDENT_PIN 5

// ---------------- Globals ----------------
WebServer server(80);
Adafruit_MPU6050 mpu;

MAX30105 particleSensor;
bool maxInitialized = false;
unsigned long lastHeartRead = 0;
int heartRate = 0;

HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

HardwareSerial dfSerial(1);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int pwmFreq = 1000;
const int pwmResolution = 8;

int speedValue = 150;
unsigned long lastSensorRead = 0;

bool accidentTriggered = false;

unsigned long lastBlink = 0;
int blinkState = 0;
bool ledOn = false;

// ============================================================
// DFPlayer Mini command helper
// ============================================================
void sendCommand(uint8_t cmd, uint8_t param) {
  uint8_t packet[10];

  packet[0] = 0x7E;
  packet[1] = 0xFF;
  packet[2] = 0x06;
  packet[3] = cmd;
  packet[4] = 0x00;
  packet[5] = 0x00;
  packet[6] = param;

  uint16_t checksum = 0xFFFF - (0xFF + 0x06 + cmd + param) + 1;
  packet[7] = (checksum >> 8);
  packet[8] = (checksum & 0xFF);
  packet[9] = 0xEF;

  dfSerial.write(packet, 10);
}

// ============================================================
// Timestamp helper — used as the key for each Firebase record
// ============================================================
String getTimeString() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return String(millis());
  }

  char buf[32];
  strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &timeinfo);
  return String(buf);
}

// ============================================================
// Push sensor + GPS snapshot to Firebase
// ============================================================
void sendToFirebase(float lat, float lon, bool gas, bool fire, int vibration, String tilt, int bpm) {
  FirebaseJson json;

  json.set("latitude", lat);
  json.set("longitude", lon);
  json.set("gas", gas);
  json.set("fire", fire);
  json.set("vibration", vibration);
  json.set("tilt", tilt);
  json.set("heartRate", bpm);

  String timeStr = getTimeString();
  String path = "/accidents/" + vehicleID + "/" + timeStr;

  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("Data stored with vehicle ID & time");
  } else {
    Serial.print("Firebase Error: ");
    Serial.println(fbdo.errorReason());
  }
}

// ============================================================
// Motor control
// ============================================================
void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ============================================================
// Web dashboard (served directly from the ESP32)
// ============================================================
String webpage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{
  text-align:center;
  font-family:Arial;
  background:#111;
  color:white;
}
button{
  width:120px;
  height:60px;
  font-size:18px;
  margin:8px;
  border-radius:10px;
  border:none;
  background:#2196F3;
  color:white;
}
button:active{
  background:red;
}
.slider{
  width:300px;
}
.audio-grid{
  display:grid;
  grid-template-columns: repeat(3, 120px);
  justify-content:center;
  gap:10px;
  margin-top:15px;
}
</style>
</head>
<body>

<h2>ESP32 CAR CONTROL</h2>

<button ontouchstart="send('forward')" onmousedown="send('forward')" onmouseup="send('stop')">FORWARD</button>
<br>
<button ontouchstart="send('left')" onmousedown="send('left')" onmouseup="send('stop')">LEFT</button>
<button ontouchstart="send('right')" onmousedown="send('right')" onmouseup="send('stop')">RIGHT</button>
<br>
<button ontouchstart="send('backward')" onmousedown="send('backward')" onmouseup="send('stop')">BACK</button>
<br><br>
<button onclick="send('stop')" style="background:red">STOP</button>
<br><br>

Speed:<br>
<input type="range" min="0" max="255" value="150" class="slider"
oninput="speed(this.value)">

<br><br>
<h3>Audio Control</h3>
<div class="audio-grid">
<button onclick="play(1)">AUDIO 1</button>
<button onclick="play(2)">AUDIO 2</button>
<button onclick="play(3)">AUDIO 3</button>
</div>

<script>
function send(cmd){
  fetch("/move?dir="+cmd);
}
function speed(val){
  fetch("/speed?value="+val);
}
function play(num){
  fetch("/play?track="+num);
}
</script>

</body>
</html>
)rawliteral";

// ============================================================
// Web server route handlers
// ============================================================
void handleRoot() {
  server.send(200, "text/html", webpage);
}

void handleMove() {
  String dir = server.arg("dir");

  if (dir == "forward") forward();
  else if (dir == "backward") backward();
  else if (dir == "left") left();
  else if (dir == "right") right();
  else stopCar();

  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  speedValue = server.arg("value").toInt();
  ledcWrite(pwmChannelA, speedValue);
  ledcWrite(pwmChannelB, speedValue);
  server.send(200, "text/plain", "OK");
}

void handlePlay() {
  int track = server.arg("track").toInt();
  sendCommand(0x03, track);
  server.send(200, "text/plain", "OK");
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(MQ2_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT);
  pinMode(VIB_PIN, INPUT);

  pinMode(ACCIDENT_PIN, OUTPUT);
  digitalWrite(ACCIDENT_PIN, LOW);

  pinMode(STATUS_LED, OUTPUT);

  ledcSetup(pwmChannelA, pwmFreq, pwmResolution);
  ledcSetup(pwmChannelB, pwmFreq, pwmResolution);
  ledcAttachPin(ENA, pwmChannelA);
  ledcAttachPin(ENB, pwmChannelB);
  stopCar();

  Wire.begin(21, 22);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 NOT FOUND");
  } else {
    Serial.println("MAX30102 ready");
  }

  if (!mpu.begin()) {
    Serial.println("MPU ERROR");
    while (1);
  }

  gpsSerial.begin(9600, SERIAL_8N1, 16, -1);
  dfSerial.begin(9600, SERIAL_8N1, 18, 19);
  delay(2000);

  sendCommand(0x06, 30);   // set DFPlayer volume
  delay(500);
  sendCommand(0x03, 4);    // play startup track

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST offset
  int retry = 0;
  const int maxRetries = 20;
  while (time(nullptr) < 100000 && retry < maxRetries) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  if (time(nullptr) < 100000) {
    Serial.println("\nTime sync FAILED");
  } else {
    Serial.println("\nTime synced!");
  }

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.test_mode = true;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  fbdo.setResponseSize(2048);

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/speed", handleSpeed);
  server.on("/play", handlePlay);
  server.onNotFound([]() {
    server.send(200, "text/plain", "OK");
  });
  server.begin();
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  server.handleClient();

  // ---------------- Status LED patterns ----------------
  if (WiFi.status() != WL_CONNECTED) {
    // Fast quad-blink = WiFi lost
    if (millis() - lastBlink > 150) {
      lastBlink = millis();
      if (blinkState == 0) {
        digitalWrite(STATUS_LED, HIGH);
        blinkState = 1;
      } else if (blinkState == 1) {
        digitalWrite(STATUS_LED, LOW);
        blinkState = 2;
      } else if (blinkState == 2) {
        digitalWrite(STATUS_LED, HIGH);
        blinkState = 3;
      } else {
        digitalWrite(STATUS_LED, LOW);
        blinkState = 0;
        lastBlink += 500;
      }
    }
  } else if (accidentTriggered) {
    // Fast blink = accident state
    if (millis() - lastBlink > 150) {
      lastBlink = millis();
      ledOn = !ledOn;
      digitalWrite(STATUS_LED, ledOn);
    }
  } else {
    // Slow blink = normal operation
    if (millis() - lastBlink > 600) {
      lastBlink = millis();
      ledOn = !ledOn;
      digitalWrite(STATUS_LED, ledOn);
    }
  }

  // ---------------- Accident state: heart-rate monitoring ----------------
  if (accidentTriggered) {
    stopCar();

    if (!maxInitialized) {
      particleSensor.setup();
      particleSensor.setPulseAmplitudeRed(0x0A);
      particleSensor.setPulseAmplitudeGreen(0);
      particleSensor.setSampleRate(100);
      particleSensor.setPulseWidth(411);
      particleSensor.setADCRange(2048);
      maxInitialized = true;
      Serial.println("Heart Rate Monitoring Started");
    }

    long irValue = particleSensor.getIR();

    if (irValue > 20000) {
      if (checkForBeat(irValue)) {
        static unsigned long lastBeat = 0;
        static int beatCount = 0;
        static long bpmSum = 0;

        long delta = millis() - lastBeat;
        lastBeat = millis();

        int currentBPM = 60 / (delta / 1000.0);
        bpmSum += currentBPM;
        beatCount++;

        if (beatCount >= 5) {
          heartRate = bpmSum / 5;
          bpmSum = 0;
          beatCount = 0;

          Serial.print("Avg BPM: ");
          Serial.println(heartRate);

          String timeStr = getTimeString();
          String path = "/accidents/" + vehicleID + "/live/heartRate/" + timeStr;
          Firebase.RTDB.setInt(&fbdo, path.c_str(), heartRate);
        }
      }
    } else {
      Serial.println("No finger");
    }
    return; // skip the normal sensor-polling block below while in accident state
  }

  // ---------------- Normal state: sensor polling ----------------
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (millis() - lastSensorRead > 700) {
    lastSensorRead = millis();

    float lat = gps.location.lat();
    float lon = gps.location.lng();
    int gasValue = analogRead(MQ2_PIN);
    int flame = digitalRead(FLAME_PIN);
    int vibration = analogRead(VIB_PIN);

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float x = a.acceleration.x;
    float y = a.acceleration.y;

    bool gasDetected = gasValue > 2000;
    bool fireDetected = flame == LOW;
    bool vibrationDetected = vibration > 2000;

    String tilt = "STABLE";
    bool tiltDetected = false;

    if (x > 6) {
      tilt = "RIGHT";
      tiltDetected = true;
    } else if (x < -6) {
      tilt = "LEFT";
      tiltDetected = true;
    } else if (y > 6) {
      tilt = "FORWARD";
      tiltDetected = true;
    } else if (y < -6) {
      tilt = "BACKWARD";
      tiltDetected = true;
    }

    Serial.println("----------------------");
    Serial.print("Latitude  : ");
    Serial.println(lat, 6);
    Serial.print("Longitude : ");
    Serial.println(lon, 6);
    Serial.println(gasDetected ? "Gas Detected: YES" : "Gas Detected: NO");
    Serial.println(fireDetected ? "Fire Detected: YES" : "Fire Detected: NO");
    Serial.print("Vibration Value: ");
    Serial.println(vibration);
    Serial.print("Tilt: ");
    Serial.println(tilt);

    if (gasDetected || fireDetected || vibrationDetected || tiltDetected) {
      accidentTriggered = true;
      Serial.println("ACCIDENT DETECTED");
      digitalWrite(ACCIDENT_PIN, HIGH);
      sendCommand(0x03, 5); // play accident alert track
    }

    if (!gps.location.isValid()) {
      Serial.println("GPS not fixed!");
    }

    sendToFirebase(lat, lon, gasDetected, fireDetected, vibration, tilt, 0);
  }
}
