#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include "EyeAnimation.h"

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
const unsigned long displayInterval = 50;      // Update OLED
const unsigned long slideInterval = 10000;     // Ganti tampilan

// ==== DATA VARIABEL ====
String cuacaSekarang = "Loading...";
String cuacaBesok = "Loading...";
String suhu = "-";
String uv = "-";
float highTemp = 0;
float lowTemp = 0;

// ==== SLIDES ====
int currentSlide = 0;

// ==== TIME ====
struct tm timeinfo;
const char* HARI[] = {"Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"};
const char* BULAN[] = {"Jan","Feb","Mar","Apr","Mei","Jun","Jul","Ags","Sep","Okt","Nov","Des"};

// ==== EYE ANIMATION INSTANCE ====
EyeAnimation eyeAnim;

// ==== FUNGSI KONVERSI CUACA ====
String getWeatherDesc(int code) {
  switch (code) {
    case 0: return "Cerah";
    case 1: case 2: return "Berawan";
    case 3: return "Mendung";
    case 61: case 63: return "Hujan";
    case 95: return "Petir";
    default: return "N/A";
  }
}

// ==== ICON CUACA (SUN) ====
static const unsigned char PROGMEM sun_icon[] = {
  0x00,0x00,0x00,0x10,0x00,0x18,0x00,0x10,0x00,0x00,0x07,0xe0,0x1f,0xf8,0x3f,0xfc,
  0x7f,0xfe,0x7f,0xfe,0xff,0xff,0xff,0xff,0x7f,0xfe,0x7f,0xfe,0x3f,0xfc,0x1f,0xf8,
  0x07,0xe0,0x00,0x00,0x00,0x10,0x00,0x18,0x00,0x10,0x00,0x00
};

// ==== ICON HUJAN ====
static const unsigned char PROGMEM rain_icon[] = {
  0x00,0x00,0x00,0x00,0x0f,0xe0,0x3f,0xf8,0x7f,0xfc,0xff,0xfe,0xff,0xfe,0x7f,0xfc,
  0x3f,0xf8,0x00,0x00,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x00,0x00,0x00,0x00
};

// ==== ICON BERAWAN ====
static const unsigned char PROGMEM cloud_icon[] = {
  0x00,0x00,0x00,0x00,0x03,0xc0,0x0f,0xf0,0x1f,0xf8,0x3f,0xfc,0x7f,0xfe,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// ==== HALAMAN ANIMASI MATA ====
void drawEyeScreen() {
  display.clearDisplay();
  
  // Update dan gambar animasi mata
  eyeAnim.update();
  eyeAnim.draw(display);
  
  // Status di bawah mata
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Nama kota
  display.setCursor(35, 46);
  display.print("Tangerang");
  
  // Status WiFi
  display.setCursor(18, 56);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("WiFi: Connected");
  } else {
    display.print("WiFi: Offline");
  }
  
  display.display();
}

// ==== HALAMAN WAKTU ====
void drawTimeScreen() {
  display.clearDisplay();

  if (!getLocalTime(&timeinfo)) return;
  
  // Border
  display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 4, SSD1306_WHITE);

  // Jam besar
  char timeStr[6];
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

  display.setTextSize(3);
  int16_t x1, y1; 
  uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 15);
  display.setTextColor(SSD1306_WHITE);
  display.print(timeStr);
  
  // Tanggal
  char dateStr[32];
  sprintf(dateStr, "%s, %d %s", HARI[timeinfo.tm_wday], timeinfo.tm_mday, BULAN[timeinfo.tm_mon]);
  display.setTextSize(1);
  display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 48);
  display.print(dateStr);
  
  display.display();
}

// ==== HALAMAN CUACA ====
void drawWeatherScreen() {
  display.clearDisplay();
  
  // Border
  display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 4, SSD1306_WHITE);
  display.drawLine(0, 14, SCREEN_WIDTH, 14, SSD1306_WHITE);
  display.drawLine(0, 46, SCREEN_WIDTH, 46, SSD1306_WHITE);
  
  // Header
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(8, 4);
  display.print("CUACA");
  display.setCursor(SCREEN_WIDTH - 58, 4);
  display.print("Tangerang");
  
  // Icon cuaca
  if (cuacaSekarang.indexOf("Hujan") >= 0) {
    display.drawBitmap(10, 20, rain_icon, 20, 20, SSD1306_WHITE);
  } else if (cuacaSekarang.indexOf("Berawan") >= 0 || cuacaSekarang.indexOf("Mendung") >= 0) {
    display.drawBitmap(10, 20, cloud_icon, 20, 20, SSD1306_WHITE);
  } else {
    display.drawBitmap(10, 20, sun_icon, 20, 20, SSD1306_WHITE);
  }
  
  // Suhu besar
  display.setTextSize(3);
  display.setCursor(38, 18);
  display.print(suhu);
  display.setTextSize(1);
  display.setCursor(68, 18);
  display.print("o");
  display.setTextSize(2);
  display.setCursor(74, 20);
  display.print("C");
  
  // Kondisi & UV
  display.setTextSize(1);
  display.setCursor(38, 38);
  display.print(cuacaSekarang);

  display.setCursor(SCREEN_WIDTH - 32, 38);
  display.print("UV:");
  display.print(uv);
  
  // Footer - Forecast besok
  display.setCursor(8, 52);
  display.print("Besok: ");
  display.print(cuacaBesok);

  display.setCursor(SCREEN_WIDTH - 44, 52);
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
  
  // WiFi Setup
  WiFiManager wm;
  Serial.println("Configuring WiFi...");
  if (!wm.autoConnect("MiniWeather-Setup")) {
    Serial.println("Gagal connect WiFi, reboot...");
    delay(2000);
    ESP.restart();
  }
  
  // OLED Setup
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED gagal diinisialisasi"));
    for(;;);
  }
  
  // Welcome Screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 28);
  display.println("WiFi Connected!");
  display.display();
  delay(2000);
  
  // Time & Weather Setup
  configTime(7 * 3600, 0, "pool.ntp.org");
  fetchData();
  
  // Initialize eye animation
  eyeAnim.begin();
}

// ==== LOOP ====
void loop() {
  unsigned long now = millis();
  
  // Fetch weather data
  if (now - lastWeatherFetch >= weatherInterval) {
    lastWeatherFetch = now;
    fetchData();
  }
  
  // Change slide
  if (now - lastSlideChange >= slideInterval) {
    lastSlideChange = now;
    currentSlide = (currentSlide + 1) % 3;
  }
  
  // Update display
  if (now - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = now;
    
    switch (currentSlide) {
      case 0: drawEyeScreen(); break;
      case 1: drawTimeScreen(); break;
      case 2: drawWeatherScreen(); break;
    }
  }
  
  delay(10);
}
