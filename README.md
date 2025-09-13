# Smart_Medibox
Medibox – An IoT-based smart medicine box using ESP32 with DHT22, LDR, and servo motor for monitoring and controlled access. Real-time data and settings are managed via MQTT on a Node-RED dashboard. The system is designed, simulated, and tested on Wokwi.


---

## 🚀 Features

⏰ Medicine Reminder – configurable alarms for medication schedules
🌡️ Temperature & Humidity Monitoring using DHT22 sensor
💡 Light Intensity Tracking with an LDR sensor
🔄 Servo Motor Control for automated medicine access
📊 Node-RED Dashboard with gauges, charts, and control sliders
📤 MQTT Communication using HiveMQ public broker
🎛️ Dynamic Configurations – set sampling/sending intervals, snooze/stop alarms, adjust ideal storage conditions
🧪 Fully Simulated on Wokwi
 (no hardware required)

---

## 📁 Repository Structure

| File/Folder         | Description                                         |
| ------------------- | --------------------------------------------------- |
| `sketch.ino`        | Main ESP32 code to handle sensors, servo, and MQTT  |
| `wokwi-project.txt` | Project ID for Wokwi simulation                     |
| `diagram.json`      | Wokwi circuit diagram (ESP32 + DHT22 + LDR + Servo) |
| `flows.json`        | Node-RED flow for UI and MQTT topics                |
| `libraries.txt`     | Required libraries (for Wokwi or Arduino IDE)       |

---






