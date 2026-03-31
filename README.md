# NowBoard

**NowBoard** is a compact, ESP32-powered smart information display that delivers real-time **time, weather, sports scores, and news headlines** on a 128×64 OLED screen. Designed for efficiency and clarity, it combines multiple data sources into a clean, animated interface suitable for desks, workspaces, or embedded projects.

---

## 📌 Overview

NowBoard transforms a simple ESP32 and OLED module into a live dashboard. The display is divided into two main sections:

* **Left Panel:** Always-on digital clock (NTP synchronized)
* **Right Panel:** Animated content carousel showing:

  * Weather updates
  * Sports scores (with team abbreviations)
  * News headlines

The system prioritizes low resource usage, smooth UI transitions, and reliable data fetching using free, publicly accessible APIs.

---

## ✨ Features

* ⏰ **Accurate Time Sync**

  * NTP-based real-time clock with timezone support

* 🌤️ **Live Weather Data**

  * Powered by Open-Meteo (no API key required)
  * Displays temperature and weather condition

* ⚽ **Sports Scores**

  * Uses TheSportsDB API
  * Displays latest match with team abbreviations and scores

* 📰 **News Headlines**

  * Fetches latest headlines via RSS (BBC News)
  * Optimized text wrapping for small displays

* 🎬 **Smooth Animations**

  * Vertical sliding transitions between panels
  * Clean, readable UI for constrained screen size

* 📟 **Lightweight & Efficient**

  * Designed for ESP32 performance constraints
  * Minimal memory footprint

---

## 🧰 Hardware Requirements

* ESP32 Development Board
* 0.96" 128×64 I2C OLED Display (NFP1315 / SSD1306 compatible)
* Jumper wires
* USB cable for programming

---

## 🔌 Wiring

| OLED Pin | ESP32 Pin |
| -------- | --------- |
| VCC      | 3.3V      |
| GND      | GND       |
| SDA      | GPIO 21   |
| SCL      | GPIO 22   |

> ⚠️ If the display does not initialize, try I2C address `0x3D` instead of `0x3C`.

---

## 📚 Software & Libraries

Install the following libraries via Arduino IDE:

* `U8g2`
* `ArduinoJson`

---

## 🌐 Data Sources

* **Time:** NTP (`pool.ntp.org`)
* **Weather:** Open-Meteo API
* **Sports:** TheSportsDB (free tier)
* **News:** BBC RSS Feed

---

## ⚙️ Configuration

Before uploading the code, update the following parameters:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

const float LATITUDE  = 23.8103;
const float LONGITUDE = 90.4125;

const char* TZ_INFO = "UTC-6";
```

You can also change the sports league:

```cpp
const char* SPORTS_LEAGUE_ID = "4328"; // Example: English Premier League
```

---

## 🖥️ Display Layout

```
+----------------+----------------------+
|                |                      |
|   TIME PANEL   |   DYNAMIC PANEL      |
|   (fixed)      |  Weather / Sports /  |
|                |  News (animated)     |
|                |                      |
+----------------+----------------------+
```

---

## 🚀 Getting Started

1. Connect the OLED display to ESP32
2. Install required libraries
3. Update Wi-Fi and location settings
4. Upload the code using Arduino IDE
5. Power the ESP32 and enjoy your live dashboard

---

## 🛠️ Troubleshooting

**Blank Display**

* Check wiring (SDA/SCL)
* Try I2C address `0x3D`
* Run an I2C scanner sketch

**Wi-Fi Not Connecting**

* Verify credentials
* Ensure 2.4GHz network (ESP32 does not support 5GHz)

**No Data Showing**

* Check internet connection
* APIs may temporarily fail; system will retry automatically

---

## 🔮 Future Improvements

* Weather icons and graphical UI enhancements
* Multiple sports events display
* Custom RSS feeds
* Touch/button navigation
* Dark/light themes

---

## 👤 Author

**Nur Tahsin**

---

## 📄 License

This project is open-source and available under the MIT License.

---

## 🤝 Contributions

Contributions, suggestions, and improvements are welcome. Feel free to fork the project and submit a pull request.
