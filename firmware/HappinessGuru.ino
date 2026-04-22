#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>

// Use ESP8266Audio library (compatible with ESP32)
#include "AudioFileSourceHTTPStream.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// Wi-Fi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Backend API
const char* serverUrl = "http://192.168.0.101:8000/api/status"; 
const char* audioUrl = "http://192.168.0.101:8000/api/audio/latest.mp3"; // Expected stream

// LCD Config (Address usually 0x27 or 0x3F, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Audio Objects
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioOutputI2S *out;

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
  wav = new AudioGeneratorWAV();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Awaiting Seekers");
}

void loop() {
  // Typical execution logic:
  // 1. You could either poll the server periodically or wait for a push notification
  //    (like MQTT) to know when a new audio file is ready. Let's assume we triggered an audio play:
  
  // Example code to start playing:
  /*
  if (shouldPlay) {
    file = new AudioFileSourceHTTPStream(audioUrl);
    wav->begin(file, out);
    lcd.clear();
    lcd.print("Spreading Wisdom");
  }
  */

  // If audio is playing, keep feeding the buffer
  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      if(file) { file->close(); delete file; file = nullptr; }
      lcd.clear();
      lcd.print("Awaiting Seekers");
    }
  }

  // Small delay for stability if not playing audio
  delay(10);
}
