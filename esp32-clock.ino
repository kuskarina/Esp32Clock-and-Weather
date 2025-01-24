#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include "time.h"

// Wi-Fi Credentials
const char* ssid = "<your_ssid>";
const char* password = "<your_password";

// Open-Meteo Weather API https://api.open-meteo.com <- go here and get the link for your location
const char* weatherApiUrl = "https://api.open-meteo.com/v1/forecast?latitude=35.2226&longitude=-97.4395&current_weather=true&temperature_unit=fahrenheit";

// NTP Configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -21600; // GMT-6 (Central Standard Time)
const int daylightOffset_sec = 3600; // Daylight Saving Time offset (if applicable)

// OLED Display (I2C) pin 21 SDA 22 SCL
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Variables for weather data
String temperature = "--";
String weatherDescription = "Fetching...";

// Timing variables
unsigned long previousMillis = 0;
const long interval = 60000; // Update every 60 seconds

void setup() {
  Serial.begin(115200);

  // Initialize OLED
  u8g2.begin();

  // Display welcome screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 30, "Initializing...");
  u8g2.sendBuffer();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi!");

  // Configure NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Synchronizing time...");
  if (!syncTime()) {
    Serial.println("Failed to synchronize time.");
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Fetch weather data
    fetchWeatherData();
  }

  // Display clock and weather
  displayClockAndWeather();
}

bool syncTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time from NTP server");
    return false;
  }
  return true;
}

void fetchWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(weatherApiUrl);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("Raw Weather API Response:");
      Serial.println(payload);

      // Parse JSON response
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("JSON Deserialization Error: ");
        Serial.println(error.c_str());
        temperature = "--";
        weatherDescription = "Parsing Error";
        return;
      }

      // Extract temperature
      temperature = String(doc["current_weather"]["temperature"].as<float>()) + " F";
      weatherDescription = "Clear"; // Open-Meteo does not provide detailed descriptions

      Serial.println("Parsed Temperature: " + temperature);
      Serial.println("Parsed Weather: " + weatherDescription);

    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpCode);
      temperature = "--";
      weatherDescription = "HTTP Error";
    }

    http.end();
  } else {
    Serial.println("Wi-Fi disconnected. Unable to fetch weather data.");
  }
}

void displayClockAndWeather() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char timeString[10];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);

  // Render on OLED
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  // Display time
  u8g2.drawStr(0, 12, "Time:");
  u8g2.drawStr(50, 12, timeString);

  // Display weather
  u8g2.drawStr(0, 30, "Temp:");
  u8g2.drawStr(50, 30, temperature.c_str());
  u8g2.drawStr(0, 50, "Weather:");
  u8g2.drawStr(0, 62, weatherDescription.c_str());

  u8g2.sendBuffer();
}


