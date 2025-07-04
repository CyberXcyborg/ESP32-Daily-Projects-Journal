# ESP32 Projects Journal

A sophisticated web-based journal application for ESP32 microcontrollers that allows you to document your daily projects, code snippets, and development notes. Built with a modern glassmorphism UI and real-time connectivity monitoring.

![ESP32 Projects Journal](https://img.shields.io/badge/ESP32-Projects%20Journal-blue?style=for-the-badge&logo=espressif)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)
![Arduino](https://img.shields.io/badge/Arduino-Compatible-teal?style=for-the-badge&logo=arduino)

## 🚀 Features

- **📅 Date-based Organization**: Organize projects by date for easy tracking
- **📝 Dual Editor Interface**: Separate editors for journal notes and code snippets
- **💾 Persistent Storage**: Uses SPIFFS for reliable data storage on ESP32
- **🌐 Modern Web Interface**: Responsive glassmorphism design with real-time updates
- **🔄 Real-time Connectivity**: Live connection status monitoring
- **⚡ Auto-save**: Smart saving with keyboard shortcuts (Ctrl+S)
- **🗑️ Project Management**: Add, delete, and manage multiple projects per day
- **📱 Mobile Responsive**: Works seamlessly on desktop and mobile devices
- **🎨 Beautiful UI**: Modern design with gradient backgrounds and smooth animations

## 📋 Requirements

### Hardware
- ESP32 development board (any variant)
- Minimum 4MB flash memory (for SPIFFS storage)

### Software
- Arduino IDE 1.8.x or newer
- ESP32 Board Package for Arduino

### Required Libraries
```cpp
#include <WiFi.h>          // ESP32 WiFi library (built-in)
#include <WebServer.h>     // ESP32 WebServer library (built-in)
#include <SPIFFS.h>        // ESP32 SPIFFS library (built-in)
#include <ArduinoJson.h>   // JSON handling library

## 🔧 Installation

### 1. Install Arduino Libraries

- Open **Arduino IDE**
- Go to **Tools → Manage Libraries**
- Search for **"ArduinoJson"** by Benoit Blanchon
- Install version **6.x.x**

### 2. Configure WiFi Credentials

Edit the WiFi credentials in the code:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

### 3. Upload to ESP32

- Connect your ESP32 to your computer
- Select the correct board and port in Arduino IDE
- Upload the code
- Open Serial Monitor to see the IP address

📱 Usage
Initial Setup
After uploading, open Serial Monitor (115200 baud)

Wait for the ESP32 to connect to WiFi

Note the IP address shown (e.g., 192.168.1.100)

Open your browser and go to:
http://[ESP32_IP_ADDRESS]

Using the Interface
Creating Projects
Select a date using the date picker

Enter a project name in the "Add New Project" field

Click "Add Project"

The project will be created and selected automatically

Writing Entries
Select a project from the dropdown

Use the Journal Entry area for notes

Use the Code Snippet area for technical content

Press Ctrl+S or click Save Entry

Managing Projects
Switch Projects: Use the dropdown

Delete Projects: Use the delete button

Date Navigation: Change dates to browse logs

Keyboard Shortcuts
Ctrl+S (or Cmd+S on Mac): Save current entry
