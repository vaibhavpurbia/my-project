# Urban Pulse | Smart IoT Traffic Monitoring System (Patent Pending)

**Status:** 🔒 Under Intellectual Property Protection

**Urban Pulse** is an IoT solution designed to classify vehicles and detect overspeeding using single-point piezoelectric sensing. This repository contains the **firmware logic** for the ESP32 node.

> **Note:** Detailed hardware schematics, circuit diagrams, and physical prototype photos are currently withheld due to ongoing patent filing processes.

## 🚀 System Capabilities
- **Edge Computing:** Classifies vehicles (Car vs. Truck vs. Cycle) locally on the ESP32 to reduce latency.
- **Pulse-Width Speed Detection:** Calculates vehicle speed based on tire contact duration.
- **Wake-on-Vibration:** Power management algorithm achieving 90% energy savings using Deep Sleep triggers.
- **Cloud Integration:** Secure, real-time data streaming to Google Firebase.

## 🛠️ Tech Stack
- **Firmware:** Embedded C++
- **Hardware:** ESP32 DevKit V1
- **Cloud:** Google Firebase (RTDB)
- **Protocols:** Wi-Fi (802.11 b/g/n), JSON Serialization

## ⚠️ License & Usage
This code is provided for portfolio demonstration purposes only. Commercial use, reproduction, or reverse engineering of the logic for hardware implementation is strictly prohibited without prior authorization.

---
*© 2026 Vaibhav Purbia*
