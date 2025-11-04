#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <queue>
#include <time.h>

// ==== DHT22 ====
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ==== OLED ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== Open-Meteo URL ====
const char* weatherURL = "https://api.open-meteo.com/v1/forecast?latitude=-6.2&longitude=106.6&hourly=temperature_2m,weathercode&timezone=Asia/Jakarta";

// ==== Interval Tasks ====
unsigned long lastIndoorRead = 0;
unsigned long lastWeatherFetch = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSlideChange = 0;

const unsigned long indoorInterval = 5000;      // baca suhu tiap 5s
const unsigned long weatherInterval = 60000;    // ambil cuaca tiap 60s
const unsigned long displayInterval = 1000;     // update display tiap 1s
const unsigned long slideInterval = 5000;       // ganti slide tiap 5s

// ==== Data ====
float indoorTemp = 0;
float outdoorTemp = 0;
float highTemp = 0;
float lowTemp = 0;
String weatherDesc = "Unknown";

// ==== Slide Control ====
int currentSlide = 0;
int eyeFrame = 0;  // untuk animasi mata

void drawEyeAnimation() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Simulasi animasi mata (bola mata bergerak kiri-kanan)
  int eyeX = 40;
  int eyeY = 32;
  int radius = 12;
  int pupilOffset = (eyeFrame % 4) * 2 - 4;  // -4, -2, 0, 2, 4

  // Mata kiri
  display.drawCircle(eyeX, eyeY, radius, SSD1306_WHITE);
  display.fillCircle(eyeX + pupilOffset, eyeY, 4, SSD1306_WHITE);

  // Mata kanan
  display.drawCircle(eyeX + 40, eyeY, radius, SSD1306_WHITE);
  display.fillCircle(eyeX + 40 + pupilOffset, eyeY, 4, SSD1306_WHITE);

  eyeFrame++;
  if (eyeFrame > 7) eyeFrame = 0;

  display.display();
}

void drawClockAndIndoor() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Waktu sekarang
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

    display.setTextSize(3);
    display.setCursor(10, 5);
    display.println(timeStr);
  } else {
    display.setTextSize(2);
    display.setCursor(20, 5);
    display.println("NoTime");
  }

  // Suhu dalam ruangan
  display.setTextSize(1);
  display.setCursor(20, 45);
  display.print("Indoor: ");
  display.print((int)indoorTemp);
  display.println("°C");

  display.display();
}

void drawWeather() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(10, 0);
  display.println("Tangerang Weather");

  display.setTextSize(3);
  display.setCursor(0, 15);
  display.print((int)outdoorTemp);
  display.println("°");

  display.setTextSize(1);
  display.setCursor(60, 25);
  display.println(weatherDesc);

  display.setCursor(10, 50);
  display.print("H:");
  display.print((int)highTemp);
  display.print(" L:");
  display.print((int)lowTemp);

  display.display();
}

void setup() {
  Serial.begin(115200);

  // ==== WiFi Manager ====
  WiFiManager wm;
  Serial.println("Configuring WiFi...");
  if (!wm.autoConnect("ESP32-Setup")) {
    Serial.println("❌ Gagal connect WiFi, reboot...");
    delay(2000);
    ESP.restart();
  }
  Serial.println("✅ WiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // ==== Inisialisasi DHT & OLED ====
  dht.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 failed");
    for (;;);
  }
  display.clearDisplay();

  configTime(7 * 3600, 0, "pool.ntp.org"); // GMT+7
}

void loop() {
  unsigned long now = millis();

  // === Task 1: Baca suhu indoor ===
  if (now - lastIndoorRead >= indoorInterval) {
    lastIndoorRead = now;
    float temp = dht.readTemperature();
    if (!isnan(temp)) indoorTemp = temp;
  }

  // === Task 2: Ambil data cuaca luar ===
  if (now - lastWeatherFetch >= weatherInterval) {
    lastWeatherFetch = now;
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(weatherURL);
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(8192);
        deserializeJson(doc, payload);

        outdoorTemp = doc["hourly"]["temperature_2m"][0].as<float>();
        highTemp = lowTemp = outdoorTemp;
        for (int i = 0; i < 24; i++) {
          float t = doc["hourly"]["temperature_2m"][i].as<float>();
          if (t > highTemp) highTemp = t;
          if (t < lowTemp) lowTemp = t;
        }

        int code = doc["hourly"]["weathercode"][0].as<int>();
        switch (code) {
          case 0: weatherDesc = "Clear"; break;
          case 1: case 2: weatherDesc = "Mostly Cloudy"; break;
          case 3: weatherDesc = "Cloudy"; break;
          case 61: case 63: weatherDesc = "Rain"; break;
          default: weatherDesc = "Unknown"; break;
        }
      }
      http.end();
    }
  }

  // === Task 3: Ganti slide tiap 5 detik ===
  if (now - lastSlideChange >= slideInterval) {
    lastSlideChange = now;
    currentSlide = (currentSlide + 1) % 3; // 0→1→2→0
  }

  // === Task 4: Tampilkan sesuai slide ===
  if (now - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = now;

    switch (currentSlide) {
      case 0: drawEyeAnimation(); break;
      case 1: drawClockAndIndoor(); break;
      case 2: drawWeather(); break;
    }
  }
}
