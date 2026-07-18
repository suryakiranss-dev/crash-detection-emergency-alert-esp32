<h1 align="center">🚗 CDEAS — Crash Detection & Smart Emergency Alert System</h1>
<p align="center">An IoT-based system that detects vehicle crashes in real time and automatically alerts emergency services with location, hazard, and driver health data.</p>

<p align="center">
<img src="https://img.shields.io/badge/Platform-ESP32-blue" />
<img src="https://img.shields.io/badge/Backend-Firebase-orange" />
<img src="https://img.shields.io/badge/Language-C%2B%2B%20-00599C" />
</p>

---

## 📖 Overview

Road accidents cause high fatalities, especially in developing countries, largely due to delayed emergency response. CDEAS addresses this by fusing multiple onboard sensors to detect crashes, fire, gas leaks, and abnormal vehicle tilt — then instantly transmits the vehicle's location and sensor data to a cloud database so emergency services can respond faster, even if the driver is unconscious.

Built as a B.Tech ECE final-year project at Model Engineering College, Thrikkakara (2025).

## 🎯 Objectives

- **Real-time crash detection** using sensor fusion for accurate, immediate accident identification
- **Automated emergency alerts** — instantly transmit location and hazard data to a live database
- **Health & environmental monitoring** — pulse/heart rate, fuel leak, and flame detection
- **Proactive early warning** for accident-prone zones

## ⚙️ How It Works

1. Sensors continuously monitor vibration, tilt, gas, flame, and heart rate
2. The ESP32 evaluates sensor thresholds to detect a potential accident
3. On detection, GPS location + sensor readings are pushed to Firebase Realtime Database under a timestamped path
4. An onboard speaker (DFPlayer Mini) plays an audio alert
5. Data is available for a connected dashboard to notify the nearest hospital, police, or fire & rescue service

## 🧩 System Architecture

![CDEAS Circuit Schematic](docs/circuit_schematic.png)

**Core controller:** ESP32 (chosen for its GPIO count, processing power, and ability to interface with multiple sensors simultaneously)

| Subsystem | Component | Purpose |
|---|---|---|
| Crash/Tilt Detection | MPU6050 | Accelerometer + gyroscope for impact and orientation |
| Impact Detection | SW420 | Vibration/shock sensing |
| Gas/Smoke Detection | MQ-2 | Detects fuel leaks and smoke |
| Fire Detection | IR Flame Sensor | Fast, low-power flame detection |
| Location Tracking | NEO-6M GPS | Real-time latitude/longitude |
| Driver Vitals | MAX30102 | Heart rate / SpO₂ monitoring |
| Vision | ESP32-CAM | Captures visual evidence on trigger |
| Audio Alerts | DF3mini + Speaker | Plays voice/audio alerts |
| Motor Control | L298N + DC Motors | Vehicle movement control (testbed) |
| Power Regulation | DC-DC Buck Converters | Stable 5V/3.3V rail from supply |
| Cloud Backend | Firebase Realtime Database | Stores accident events, live heart rate, and location by vehicle ID |

## 🛠️ Tech Stack

**Firmware:** C++ (Arduino framework), PlatformIO / Arduino IDE
**Connectivity:** WiFi (ESP32 WebServer), Firebase ESP Client
**Libraries:** Adafruit MPU6050, TinyGPS++, MAX30105, Firebase_ESP_Client
**Frontend/Dashboard:** Web-based control panel (HTML/JS served from ESP32) + Firebase-backed admin dashboard
**Version Control:** Git & GitHub

## 📂 Repository Structure

```
crash-detection-smart-emergency-alert-esp32/
├── firmware/
│   └── cdeas_main.cpp          # ESP32 firmware (sensor fusion, alert logic, Firebase push)
├── docs/
│   ├── circuit_schematic.png   # Full circuit diagram
│   └── block_diagram.png       # System block diagram
├── report/
│   └── CDEAS_Project_Report.pdf
└── README.md
```

## 🚦 Applications

- Public transport & school/college buses
- Private and commercial vehicles
- Passenger safety systems (elderly, children)
- Hazardous or remote route vehicles

## ✅ Milestones Completed

- Sensor procurement & individual testing
- Dashboard UI and database design
- Real-time data transfer from ESP32 to Firebase
- Hardware assembly and wiring
- End-to-end crash detection → alert pipeline demo

## 🔭 Future Scope

- AI/ML-based crash severity prediction
- Integration with government emergency-response APIs
- Compatibility layer for autonomous vehicles

## 👥 Team

| Member | Role |
|---|---|
| Suryakiran S S | Hardware assembly, firmware development, wiring, power management, integration |
| A P Joyal | Research, report & documentation, final system testing |
| Sanjay K B | Web development, sensor data handling, GPS formatting |
| Sharafas Muhammad T M | Database records, GPS mapping |

*Guided by Mr. Manojkumar P, Assistant Professor, Dept. of Electronics Engineering*

