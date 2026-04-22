#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Wire.h>

// Use ESP8266Audio library (compatible with ESP32)
#include "AudioFileSourceHTTPStream.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// Wi-Fi Credentials
const char *ssid = "pogo";
const char *password = "YOUR_WIFI_PASSWORD";

// Backend API
const char* serverUrl = "https://happiness-guriu.onrender.com/api/status";
const char* audioUrl = "https://happiness-guriu.onrender.com/api/audio/latest.mp3"; // Expected stream

// LCD Config (Address usually 0x27 or 0x3F, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Audio Objects
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file = nullptr;
AudioOutputI2S *out = nullptr;

String lastPlayedId = "none";
unsigned long lastPollTime = 0;

void setup() {
  Serial.begin(115200);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Happiness Guru");
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");

  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Guru Online.");
  lcd.setCursor(0, 1);
  lcd.print("IP: ");
  lcd.print(WiFi.localIP());

  delay(2000);

  // Setup Audio Output
  // ESP32 Internal DAC is on Pins 25 (Channel 1) and 26 (Channel 2).
  out = new AudioOutputI2S(0, 1); // 1 = internal DAC on ESP32
  mp3 = new AudioGeneratorMP3();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Awaiting Seekers");
}

void loop() {
  // Typical execution logic:
  // Polling logic: Check backend every 3 seconds
  if (millis() - lastPollTime > 3000) {
    lastPollTime = millis();
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      int httpCode = http.GET();
      if (httpCode == 200) {
        String payload = http.getString();
        // Manually parse JSON for "latest_id"
        int idx = payload.indexOf("\"latest_id\": \"");
        if (idx != -1) {
          int endIdx = payload.indexOf("\"", idx + 14);
          String latest_id = payload.substring(idx + 14, endIdx);
          
          if (latest_id != "none" && latest_id != lastPlayedId) {
            lastPlayedId = latest_id;
            
            // Start playing!
            if (file) { file->close(); delete file; file = nullptr; }
            if (mp3 && mp3->isRunning()) mp3->stop();
            
            file = new AudioFileSourceHTTPStream(audioUrl);
            mp3->begin(file, out);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Spreading Wisdom");
          }
        }
      }
      http.end();
    }
  }

  // If audio is playing, keep feeding the buffer
  if (mp3 && mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      if (file) {
        file->close();
        delete file;
        file = nullptr;
      }
      lcd.clear();
      lcd.print("Awaiting Seekers");
    }
  }

  // Small delay for stability if not playing audio
  delay(10);
}
