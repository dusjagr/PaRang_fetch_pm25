#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h"

#define PM_SENSOR_ID "85707"    // Particulate matter sensor
#define TEMP_SENSOR_ID "85708"  // Temperature sensor
#define API_HOST "data.sensor.community"
#define API_PATH "/airrohr/v1/sensor/"
#define MAX_RETRIES 3
#define RETRY_DELAY 1000  // 1 second between retries
#define HTTP_TIMEOUT 10000 // 10 seconds timeout

bool initialFetchDone = false;  // Flag to track initial fetch

// Web Server on port 80
WebServer server(80);

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Global variables to store latest readings
float lastPM25 = 0;
float lastTemperature = 0;
String lastUpdateTime = "";

// Function to convert UTC timestamp to Thailand time (UTC+7)
String adjustToLocalTime(const char* timestamp) {
    // Parse timestamp (format: "2025-02-16 03:11:56")
    int year, month, day, hour, minute, second;
    sscanf(timestamp, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    
    // Add 7 hours for Thailand timezone
    hour += 7;
    
    // Handle day changes
    if (hour >= 24) {
        hour -= 24;
        day += 1;
    }
    
    // Format the adjusted time
    char localTime[20];
    sprintf(localTime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    return String(localTime);
}

// Function to fetch sensor data
void fetchSensorData(const char* sensorId, bool isPMSensor) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + String(API_HOST) + String(API_PATH) + String(sensorId) + "/";
    
    if (isPMSensor) {
      Serial.print("Fetching PM2.5 data");
    } else {
      Serial.print("Fetching Temperature data");
    }
    
    // Set timeout to 10 seconds
    http.setTimeout(HTTP_TIMEOUT);
    
    // Retry mechanism
    int retries = 0;
    int httpResponseCode;
    
    while (retries < MAX_RETRIES) {
      if (retries > 0) {
        Serial.print("\nRetry #");
        Serial.print(retries);
        Serial.print(" of ");
        Serial.print(MAX_RETRIES - 1);
        Serial.print("... ");
        delay(RETRY_DELAY);
      }
      
      http.begin(url);
      httpResponseCode = http.GET();
      
      if (httpResponseCode > 0) {
        Serial.println(" done!");
        String payload = http.getString();
        
        // Allocate JsonDocument
        DynamicJsonDocument doc(8192);
        
        // Parse JSON
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }
        
        // Get the first (most recent) reading
        JsonObject firstReading = doc[0];
        if (!firstReading.isNull()) {
          JsonArray sensorValues = firstReading["sensordatavalues"];
          const char* timestamp = firstReading["timestamp"];
          
          // Get location data
          JsonObject location = firstReading["location"];
          if (!location.isNull()) {
              String localTime = adjustToLocalTime(timestamp);
              
              // Print location info only on initial fetch
              if (isPMSensor && !initialFetchDone) {
                  Serial.println("----------------------------------------");
                  Serial.println("Location: Pa Rang Cafe & Art Stay");
                  Serial.println("Chiang Mai, Thailand");
                  Serial.print("GPS: ");
                  Serial.print(location["latitude"].as<float>(), 6);
                  Serial.print(", ");
                  Serial.print(location["longitude"].as<float>(), 6);
                  Serial.print(" (Alt: ");
                  Serial.print(location["altitude"].as<float>());
                  Serial.println("m)");
                  Serial.println("Timezone: UTC+7 (Indochina Time)");
                  Serial.println("----------------------------------------");
                  initialFetchDone = true;  // Mark initial fetch as done
              }
              
              // Only print the values we're interested in
              for (JsonObject value : sensorValues) {
                  const char* value_type = value["value_type"];
                  const char* value_str = value["value"];
                  
                  if (isPMSensor) {
                      if (strcmp(value_type, "P2") == 0) {
                          Serial.println("----------------------------------------");
                          Serial.print(localTime);
                          Serial.print(" | PM2.5: ");
                          Serial.print(value_str);
                          Serial.println(" µg/m³");
                          Serial.println("----------------------------------------");
                          lastPM25 = atof(value_str);
                          lastUpdateTime = localTime;
                          break;
                      }
                  } else {
                      if (strcmp(value_type, "temperature") == 0) {
                          Serial.println("----------------------------------------");
                          Serial.print(localTime);
                          Serial.print(" | Temperature: ");
                          Serial.print(value_str);
                          Serial.println(" °C");
                          Serial.println("----------------------------------------");
                          lastTemperature = atof(value_str);
                          lastUpdateTime = localTime;
                          break;
                      }
                  }
              }
          }
        }
        break;  // Success, exit retry loop
      } else {
        // Print specific error based on code
        Serial.print("\nError: ");
        switch (httpResponseCode) {
          case -1: Serial.println("Connection failed"); break;
          case -2: Serial.println("Server not found"); break;
          case -3: Serial.println("Connection timed out"); break;
          case -4: Serial.println("Connection lost"); break;
          case -5: Serial.println("No or invalid response"); break;
          case -6: Serial.println("Invalid response length"); break;
          case -7: Serial.println("Connection refused"); break;
          case -8: Serial.println("Invalid request"); break;
          case -9: Serial.println("Client timeout"); break;
          case -10: Serial.println("Invalid response"); break;
          case -11: Serial.println("Connection reset"); break;
          default: Serial.println("Unknown error"); break;
        }
      }
      
      http.end();
      retries++;
    }
  }
}

void handleRoot() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        server.send(404, "text/plain", "File not found");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void handleData() {
    StaticJsonDocument<200> doc;
    doc["pm25"] = lastPM25;
    doc["temperature"] = lastTemperature;
    doc["timestamp"] = lastUpdateTime;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleData);
    server.begin();
    Serial.println("HTTP server started");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\nStarting Air Quality Monitor...");

    // Initialize LittleFS
    if(!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    Serial.println("LittleFS Mounted Successfully");

    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    Serial.print(" SSID: ");
    Serial.println(WIFI_SSID);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect to WiFi!");
        Serial.print("Status code: ");
        Serial.println(WiFi.status());
        return;
    }

    // Initialize NTP
    timeClient.begin();
    timeClient.setTimeOffset(25200); // UTC+7 for Thailand
    Serial.println("NTP Client initialized");

    // Setup web server
    setupWebServer();

    // Reset the initial fetch flag when starting up
    initialFetchDone = false;
    
    // Fetch initial data from both sensors
    Serial.println("\nInitial data fetch:");
    fetchSensorData(PM_SENSOR_ID, true);
    delay(1000);
    fetchSensorData(TEMP_SENSOR_ID, false);
    Serial.println("\nNext update in 60 seconds...");
}

unsigned long lastFetchTime = 0;

void loop() {
    server.handleClient();  // Handle web server requests
    
    unsigned long currentMillis = millis();
    if (currentMillis - lastFetchTime >= 60000) {  // Fetch every 60 seconds
        fetchSensorData(PM_SENSOR_ID, true);
        delay(1000);
        fetchSensorData(TEMP_SENSOR_ID, false);
        Serial.println("\nNext update in 60 seconds...");
        lastFetchTime = currentMillis;
    }
    
    delay(10);  // Small delay to prevent overwhelming the CPU
}
