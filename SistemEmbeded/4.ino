#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

// ==== OLED CONFIG ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 8
#define OLED_SCL 9
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== API URL (Koordinat Tangerang) ====
const char* weatherURL = "https://api.open-meteo.com/v1/forecast?latitude=-6.1783&longitude=106.6319&hourly=temperature_2m,weathercode,uv_index&daily=weathercode,temperature_2m_max,temperature_2m_min&timezone=Asia%2FJakarta";

// ==== TASK TIMING ====
unsigned long lastWeatherFetch = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSlideChange = 0;
const unsigned long weatherInterval = 900000; // 15 menit
const unsigned long displayInterval = 1000;   // Update OLED
const unsigned long slideInterval = 10000;    // Ganti tampilan

// ==== DATA VARIABEL ====
String cuacaSekarang = "Loading...";
String cuacaBesok = "Loading...";
String suhu = "-";
String uv = "-";
float highTemp = 0;
float lowTemp = 0;

// ==== SLIDES ====
int currentSlide = 0;
int eyeFrame = 0;

// ==== TIME ====
struct tm timeinfo;
const char* HARI[]  = {"Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"};
const char* BULAN[] = {"Jan","Feb","Mar","Apr","Mei","Jun","Jul","Ags","Sep","Okt","Nov","Des"};

// ==== FUNGSI KONVERSI CUACA ====
String getWeatherDesc(int code) {
  switch (code) {
    case 0: return "Cerah";
    case 1: case 2: return "Berawan";
    case 3: return "Mendung";
    case 61: case 63: return "Hujan";
    case 95: return "Petir";
    default: return "Tidak Diketahui";
  }
}

// ==== ANIMASI MATA ====
void drawEyeAnimation() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  int eyeY = 22;
  int eyeX1 = 40;
  int eyeX2 = 80;
  int radius = 12;
  int pupilOffset = (eyeFrame % 4) * 2 - 4;

  display.drawCircle(eyeX1, eyeY, radius, SSD1306_WHITE);
  display.fillCircle(eyeX1 + pupilOffset, eyeY, 4, SSD1306_WHITE);

  display.drawCircle(eyeX2, eyeY, radius, SSD1306_WHITE);
  display.fillCircle(eyeX2 + pupilOffset, eyeY, 4, SSD1306_WHITE);

  eyeFrame++;
  if (eyeFrame > 7) eyeFrame = 0;

  display.setTextSize(1);
  display.setCursor(40, 46);
  display.print("Tangerang");

  display.setCursor(22, 56);
  if (WiFi.status() == WL_CONNECTED) display.print("WiFi: Connected");
  else display.print("WiFi: Offline");

  display.display();
}

// ==== HALAMAN WAKTU ====
void drawTimeScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (!getLocalTime(&timeinfo)) return;

  char timeStr[6];
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(3);
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 10);
  display.print(timeStr);

  char dateStr[32];
  sprintf(dateStr, "%s, %d %s", HARI[timeinfo.tm_wday], timeinfo.tm_mday, BULAN[timeinfo.tm_mon]);
  display.setTextSize(1);
  display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 47);
  display.print(dateStr);

  display.display();
}

// ==== ICON MATAHARI ====
static const unsigned char PROGMEM sun_icon[] = {
  0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x3c,0x00,0x00,0x7e,0x00,0x01,0xff,0x80,
  0x03,0xff,0xc0,0x07,0xff,0xe0,0x07,0xff,0xe0,0x0f,0xff,0xf0,0x0f,0xff,0xf0,0x0f,
  0xff,0xf0,0x07,0xff,0xe0,0x07,0xff,0xe0,0x03,0xff,0xc0,0x01,0xff,0x80,0x00,0x7e,
  0x00,0x00,0x3c,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00
};

// ==== HALAMAN CUACA ====
void drawWeatherScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // === Bingkai Luar ===
  display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 4, SSD1306_WHITE);
  display.drawLine(0, 12, SCREEN_WIDTH, 12, SSD1306_WHITE);
  display.drawLine(0, 50, SCREEN_WIDTH, 50, SSD1306_WHITE);

  // === Header ===
  display.setTextSize(1);
  display.setCursor(5, 2);
  display.print("Weather");
  display.setCursor(SCREEN_WIDTH - 60, 2);
  display.print("Tangerang");

  // === Icon Cuaca ===
  display.drawBitmap(8, 20, sun_icon, 24, 24, SSD1306_WHITE);

  // === Suhu ===
  display.setTextSize(2);
  display.setCursor(40, 18);
  display.print(suhu);
  display.drawCircle(64, 18, 2, SSD1306_WHITE);
  display.print("C");

  // === Cuaca & UV ===
  display.setTextSize(1);
  display.setCursor(40, 38);
  display.print(cuacaSekarang);
  display.setCursor(100, 38);
  display.print("UV:");
  display.print(uv);

  // === Garis bawah & Forecast ===
  display.setCursor(6, 54);
  display.print("Besok:");
  display.print(" ");
  display.print(cuacaBesok);
  display.setCursor(88, 54);
  display.print("H:");
  display.print((int)highTemp);
  display.print(" L:");
  display.print((int)lowTemp);

  display.display();
}

// ==== FETCH DATA OPEN-METEO ====
void fetchData() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(weatherURL);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, payload)) return;

    float temp = doc["hourly"]["temperature_2m"][0];
    int uvIndex = doc["hourly"]["uv_index"][0];
    int codeNow = doc["hourly"]["weathercode"][0];
    int codeTomorrow = doc["daily"]["weathercode"][1];

    cuacaSekarang = getWeatherDesc(codeNow);
    cuacaBesok = getWeatherDesc(codeTomorrow);
    suhu = String((int)round(temp));
    uv = String(uvIndex);
    highTemp = doc["daily"]["temperature_2m_max"][1];
    lowTemp = doc["daily"]["temperature_2m_min"][1];
  }
  http.end();
}

// ==== SETUP ====
void setup() {
  Serial.begin(115200);

  WiFiManager wm;
  Serial.println("Configuring WiFi...");
  if (!wm.autoConnect("MiniWeather-Setup")) {
    Serial.println("Gagal connect WiFi, reboot...");
    delay(2000);
    ESP.restart();
  }

  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED gagal diinisialisasi"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("WiFi Connected!");
  display.display();
  delay(1000);

  configTime(7 * 3600, 0, "pool.ntp.org");
  fetchData();
}

// ==== LOOP ====
void loop() {
  unsigned long now = millis();

  if (now - lastWeatherFetch >= weatherInterval) {
    lastWeatherFetch = now;
    fetchData();
  }

  if (now - lastSlideChange >= slideInterval) {
    lastSlideChange = now;
    currentSlide = (currentSlide + 1) % 3;
  }

  if (now - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = now;
    switch (currentSlide) {
      case 0: drawEyeAnimation(); break;
      case 1: drawTimeScreen(); break;
      case 2: drawWeatherScreen(); break;
    }
  }

  delay(100);
}
