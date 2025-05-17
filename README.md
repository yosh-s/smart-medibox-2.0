# Smart Medibox 2.0


*Empowering Health with Intelligent Medication Management*

Welcome to **Smart Medibox 2.0**, an innovative IoT-based solution designed to revolutionize medication adherence and environmental monitoring. Built on the ESP32 platform, this project integrates advanced sensors, a user-friendly OLED interface, MQTT communication, and servo motor control to ensure timely medication reminders and optimal storage conditions.

---

## Table of Contents
- [Project Overview](#project-overview)
- [Key Features](#key-features)
- [Functionalities](#functionalities)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Setup Instructions](#setup-instructions)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgements](#acknowledgements)

---

## Project Overview
Smart Medibox 2.0 is a comprehensive medication management system that combines real-time environmental monitoring, automated reminders, and remote control capabilities. It ensures medications are taken on time and stored under optimal conditions by monitoring temperature, humidity, and light intensity. The system communicates via MQTT for seamless integration with dashboards, supports scheduled alarms, and features an intuitive interface for user interaction.

---

## Key Features
- **Medication Reminders**: Configurable alarms with snooze functionality to remind users to take their medications.
- **Environmental Monitoring**: Tracks temperature, humidity, and light intensity to ensure safe storage conditions.
- **Remote Control**: MQTT-based communication for controlling the Medibox via a dashboard.
- **Servo Motor Control**: Automatically adjusts based on environmental conditions for precise medication dispensing.
- **OLED Display**: Displays time, date, environmental data, and system status in real-time.
- **WiFi Connectivity**: Syncs with NTP servers for accurate timekeeping and connects to MQTT brokers.
- **User-Friendly Interface**: Push-button navigation for setting alarms, time zones, and viewing schedules.
- **Warning System**: Alerts users with LEDs and a buzzer for critical temperature or humidity levels.


![image](https://github.com/user-attachments/assets/d4a7f9a4-54c6-4602-9e40-91ec5b275111)

## Functionalities
### 1. Alarm Management
- **Set Alarms**: Configure up to two daily alarms or a one-time scheduled alarm.
- **Snooze Option**: Postpone alarms by 5 minutes with a single button press.
- **Delete Alarms**: Easily remove alarms via the menu.
- **View Alarms**: Display current alarm settings on the OLED screen.

### 2. Environmental Monitoring
- **DHT22 Sensor**: Measures temperature and humidity, with warnings for values outside safe ranges (T: 24–32°C, H: 65–80%).
- **LDR Sensor**: Monitors light intensity to protect light-sensitive medications.
- **Data Publishing**: Sends temperature, humidity, and light intensity data to an MQTT broker for remote monitoring.

### 3. Servo Motor Control
- Adjusts the servo angle based on light intensity, temperature, and configurable parameters (minimum angle, control factor, ideal temperature).
- Ensures precise medication dispensing or compartment access.

### 4. MQTT Integration
- Subscribes to topics for remote control of system power, sampling intervals, and scheduled alarms.
- Publishes environmental data for real-time dashboard updates.
- Supports topics like `MEDI_ON_OFF`, `LDR_DATA`, `MEDI_TEMP`, and more.

### 5. User Interface
- **OLED Display**: Shows time, date, environmental readings, and system alerts.
- **Push Buttons**: Navigate menus to set time zones, alarms, and view schedules.
- **Melody Alerts**: Plays a musical tune during alarms for user attention.

### 6. Time Management
- Syncs with NTP servers for accurate timekeeping.
- Allows manual time zone adjustments for global use.
- Displays date and time in a clear format (YYYY-MM-DD, HH:MM:SS).

### 7. System Control
- **Power On/Off**: Toggle the system remotely via MQTT or locally via buttons.
- **LED Indicators**: Visual feedback for system status and warnings.
- **Buzzer Alerts**: Audible notifications for alarms and environmental warnings.

---

## Hardware Requirements
- **ESP32 Development Board**: Core microcontroller for processing and connectivity.
- **DHT22 Sensor**: For temperature and humidity monitoring.
- **LDR Sensor**: For light intensity measurement.
- **Servo Motor**: For automated medication dispensing.
- **SSD1306 OLED Display (128x64)**: For user interface.
- **Buzzer**: For audible alerts.
- **LEDs (2)**: For visual indicators.
- **Push Buttons (4)**: For user input (OK, Cancel, Up, Down).
- **Resistors and Breadboard**: For circuit assembly.

![image](https://github.com/user-attachments/assets/bbdd78be-8ee2-4582-8598-2a797cef4df5)


## Software Requirements
- **Arduino IDE**: For programming the ESP32.
- **Libraries**:
  - `WiFi.h`
  - `PubSubClient.h`
  - `DHTesp.h`
  - `Adafruit_GFX.h`
  - `Adafruit_SSD1306.h`
  - `ESP32Servo.h`
- **MQTT Broker**: E.g., `test.mosquitto.org` or a local broker.
- **WiFi Network**: For internet connectivity.

---

## Setup Instructions
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/yosh-s/smart-medibox-2.0.git
   ```
2. **Install Arduino IDE**:
   - Download and install from [arduino.cc](https://www.arduino.cc/en/software).
3. **Install Libraries**:
   - Open Arduino IDE, go to `Sketch > Include Library > Manage Libraries`, and install the required libraries.
4. **Connect Hardware**:
   - Wire the ESP32, DHT22, LDR, servo motor, OLED display, buzzer, LEDs, and push buttons as per the pin definitions in the code (e.g., BUZZER: GPIO 5, DHTPIN: GPIO 12).
5. **Configure WiFi**:
   - Update the WiFi credentials in the `setupWifi()` function (default: SSID: `Wokwi-GUEST`, Password: `""`).
6. **Upload Code**:
   - Open the `.ino` file in Arduino IDE, select your ESP32 board, and upload the code.
7. **Set Up MQTT**:
   - Ensure the MQTT broker is accessible (e.g., `test.mosquitto.org:1883`).
   - Update the client ID in the code if needed to avoid conflicts.
8. **Test the System**:
   - Power on the Medibox, verify the OLED display, and test alarms and MQTT communication.

---

## Usage
- **Power On**: The system connects to WiFi, syncs time, and displays the home screen.
- **Navigate Menu**:
  - Press **OK** to enter the menu.
  - Use **Up/Down** to select options (Set Time Zone, Set Alarms, View Alarms, View Scheduled, Delete Alarm, Exit).
  - Press **OK** to confirm, **Cancel** to exit.
- **Set Alarms**: Configure daily or scheduled alarms via the menu.
- **Monitor Environment**: View real-time temperature, humidity, and alerts on the OLED.
- **Remote Control**: Use an MQTT dashboard to toggle power, set schedules, or adjust parameters.
- **Servo Operation**: The servo adjusts automatically based on environmental conditions.

---

## Contributing
We welcome contributions to enhance Smart Medibox 2.0! To contribute:
1. Fork the repository.
2. Create a new branch (`git checkout -b feature-name`).
3. Make your changes and commit (`git commit -m 'Add feature'`).
4. Push to the branch (`git push origin feature-name`).
5. Open a Pull Request.

Please ensure your code follows the project's coding standards and includes appropriate documentation.

---

## License
This project is licensed under the [MIT License](LICENSE). Feel free to use, modify, and distribute as per the license terms.

---

## Acknowledgements
- **Arduino Community**: For providing robust libraries and documentation.
- **MQTT**: For enabling seamless IoT communication.
- **Contributors**: Thanks to everyone who tests, suggests, or improves this project!

---

*Stay healthy, stay on time with Smart Medibox 2.0!*
