 🔌 Smart Switchboard

An intelligent, modern replacement for traditional switchboards with touch controls, environmental sensing, and remote management capabilities.


 📌 Overview

The   Smart Switchboard   is a modern prototype aiming to replace traditional switchboards with intelligent, touch-sensitive, and remote-controllable units. Built using   ESP32 and C++ (Arduino)  , it integrates environmental sensing and automation to enhance comfort, energy efficiency, and safety.

 🧠 Key Features

* 🔐   Secure Storage Compartment   for phones, keys, or wallets
* 🌡️   Fan Speed Control   based on computed Heat Index
* ⚡   Capacitive Touch Switches   for modern interaction
* 👁️   Motion Sensing   for automatic light activation
* 🔌   Inbuilt Type-C Charging Port  
* 📱   Mobile Web Interface   for remote control and monitoring

   🔬 Technical Design

  # 🔥 Heat Index Computation

A pseudo-ML logic computes   Heat Index (HI)   in real-time using:

```
HI = -8.784695 + 1.61139411T + 2.338549R - 0.14611605TR - 0.012308094T² - 0.01642482R² + 0.002211732T²R + 0.00072546TR² - 0.000003582T²R²
```

Where:
*   T   = Temperature (°C)
*   R   = Relative Humidity (%)

Fan speed is adjusted based on a hardcoded decision logic mapping HI to speed levels (0–4).

  # ⚙️ Functional Modes

     🌟 Light Control

| Mode | Behavior |
|------|----------|
| OFF | Always off |
| AUTO | Turns on with motion; auto-off after timeout |
| FORCE ON | Always on, regardless of motion |

     🌀 Fan Control

| Mode | Behavior |
|------|----------|
| OFF | Always off |
| AUTO | Speed auto-adjusts via Heat Index |
| MANUAL | User selects level: 0%, 25%, 50%, 75%, 100% |

   💻 Technology Stack

* 🧠   Microcontroller  : ESP32
* 🧮   Programming  : C++ (Arduino)
* 🌐   Interface  : HTML, CSS, JS (served via ESP32)
* 🔌   Sensors  : DHT22 (Temp/Humidity), PIR (Motion)
* 💡   UI  : Capacitive touch, 7-segment display
* 🛠️   CAD  : Fusion 360

   🧾 Code Architecture

  # 1. Arduino Code (`.ino` / `.cpp`)
* Includes: `WiFi.h`, `ESPAsyncWebServer.h`, `DHT.h`, `ArduinoJson.h`
* Functions:
   * `setup()`: Initializes sensors, server, display
   * `loop()`: Main logic – reads sensors, checks state, updates outputs
   * Helper functions: debounce touch, read motion, compute HI, control fan/light

  # 2. Embedded HTML/CSS/JS
* Stored as `const char*` in flash
* Displays status dashboard
* Handles touch-based control toggles
* Uses JavaScript `fetch()` for async updates via JSON

  # 3. Web Server Endpoints
* `/status`: Returns current sensor values and mode states (JSON)
* `/control`: Accepts control updates (POST)
* `/`: Serves UI page

   📁 Project Structure

```
smart-switchboard/
├── code/
│   ├── main_controller.ino
│   ├── web_interface.h
│   └── helpers.cpp/.h

├── images/
│   └── renders and photos
├── docs/
│   ├── report.pdf
│   ├── poster.pdf
│   └── bill_of_materials.xlsx
└── README.md
```

   🚀 Future Additions

* 🧠   ML Integration  : Collect user feedback to train lightweight models
* 📊   Usage Analytics  : Record usage and environmental trends
* 🔋   Power Efficiency  : Predict low-usage periods to save energy
* ☁️   Cloud Sync  : Optional cloud dashboard and voice assistant support

   👨‍🔧 Team Members

*   Rahul   – Mechanical Engineering | CAD & fan speed regulation Logic & UI Devlopment
*   Susmith   – Mechanical Engineering | CAD Design
*   Jay Surya   – Smart Manufacturing Engineering | Assembly of components
*   Punneeth   – ECE | Circuitry, sensor integration, and helped combine Arduino code with fan speed control
*   Jayenth   – ECE | Firmware and Communication

