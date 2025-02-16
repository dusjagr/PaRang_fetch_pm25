# PaRang Fetch PM2.5

## Overview
This project is designed to monitor and fetch PM2.5 particulate matter data using sensors. It utilizes various libraries to connect to WiFi, handle HTTP requests, and process JSON data.

## Features
- Fetches data from PM2.5 and temperature sensors.
- Web server to display sensor data.
- Adjusts timestamps to local time (Thailand, UTC+7).

## Data Source
This project fetches data from [sensor.community](https://sensor.community/), which provides real-time air quality data from various sensors around the world.

## Hardware Used
This project utilizes an ESP32 microcontroller, which provides WiFi connectivity and sufficient processing power for handling sensor data. The PM2.5 sensor is used to measure particulate matter in the air, while a temperature sensor provides additional environmental data.

## Sensors Used
1. **BMP280**: Measures temperature and pressure.
2. **SPS30**: Measures particulate matter concentrations.

## Location
The sensors are deployed at Pa Rang Cafe, located in Thailand at an altitude of 316.7 meters. The exact coordinates are approximately 18.7913° N latitude and 98.9798° E longitude.

## Libraries Used
- Arduino
- WiFi
- WebServer
- HTTPClient
- ArduinoJson
- LittleFS
- NTPClient

## Setup
1. Clone the repository.
2. Install the required libraries in your Arduino IDE.
3. Configure your WiFi settings in `config.h`.
4. Upload the code to your ESP32 or compatible board.

## Usage
- Access the web server at your device's IP address to view live sensor data.

## License
This project is licensed under the MIT License.
