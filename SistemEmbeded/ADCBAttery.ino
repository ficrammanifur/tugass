#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <queue>

// ---------- CONFIGURATION ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 5          // Pin DHT22
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define BAT_PIN 36        // ADC pin battery (via voltage divider)

#define WEATHER_UPDATE_INTERVAL 10 * 60 * 1000UL // 10 menit
#define TEMP_UPDATE_INTERVAL 2 * 1000UL          // 2 detik
#define DISPLAY_UPDATE_INTERVAL 1000UL           // 1 detik

#define LATITUDE -6.2146
#define LONGITUDE 106.8451
#define TIMEZONE "Asia/Jakarta"

// ---------- GLOBAL VARIABLES ----------
unsigned long lastWeatherUpdate = 0;
unsigned long lastTempUpdate = 0;
unsigned long lastDisplayUpdate = 0;

float indoorTemp = 0;
float indoorHum = 0;

struct WeatherData {
  String description;
  float tempHigh;
  float tempLow;
  float tempNow;
};
std::queue<WeatherData> weatherQueue;
WeatherData currentWeather;

// ---------- FUNCTION PROTOTYPES ----------
float readBattery();
void updateIndoorTemp();
void updateWeather();
void drawOLED();

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  dht.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  display.clearDisplay();
  display.display();

  // Initial dummy weather
  currentWeather = {"Loading...", 0, 0, 0};
}

// ---------- MAIN LOOP ----------
void loop() {
  unsigned long now = millis();

  // --- Update Indoor Temp ---
  if(now - lastTempUpdate >= TEMP_UPDATE_INTERVAL){
    updateIndoorTemp();
    lastTempUpdate = now;
  }

  // --- Update Weather (every 10 menit) ---
  if(now - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL){
    updateWeather();
    lastWeatherUpdate = now;
  }

  // --- Update Display (every 1 detik) ---
  if(now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL){
    drawOLED();
    lastDisplayUpdate = now;
  }
}

// ---------- FUNCTIONS ----------

// Read battery voltage & convert to %
float readBattery(){
  int raw = analogRead(BAT_PIN);
  float voltage = raw * 3.3 / 4095.0;
  voltage *= 2; // voltage divider 1:1
  float percentage = (voltage - 3.0) / (4.2 - 3.0) * 100.0;
  if(percentage>100) percentage=100;
  if(percentage<0) percentage=0;
  return percentage;
}

// Update indoor temp & humidity
void updateIndoorTemp(){
  indoorTemp = dht.readTemperature();
  indoorHum = dht.readHumidity();
}

// Update weather from Open-Meteo API
void updateWeather(){
  if((WiFi.status() == WL_CONNECTED) || true){ // ESP32 harus dihubungkan WiFi
    HTTPClient http;
    String url = String("https://api.open-meteo.com/v1/forecast?latitude=") +
                 LATITUDE + "&longitude=" + LONGITUDE +
                 "&hourly=temperature_2m,precipitation,cloudcover&timezone=" + TIMEZONE;
    http.begin(url);
    int httpCode = http.GET();
    if(httpCode == 200){
      String payload = http.getString();
      DynamicJsonDocument doc(8192);
      DeserializationError error = deserializeJson(doc, payload);
      if(!error){
        // Ambil data jam pertama
        float tempNow = doc["hourly"]["temperature_2m"][0];
        float cloudcover = doc["hourly"]["cloudcover"][0];
        float tempHigh = tempNow + 5; // perkiraan
        float tempLow = tempNow - 4;  // perkiraan
        String desc = "Mostly Cloudy";
        if(cloudcover < 20) desc = "Sunny";
        else if(cloudcover < 50) desc = "Partly Cloudy";
        else if(cloudcover < 80) desc = "Mostly Cloudy";
        else desc = "Rain";

        WeatherData wd = {desc, tempHigh, tempLow, tempNow};
        if(weatherQueue.size() >= 1) weatherQueue.pop();
        weatherQueue.push(wd);
        currentWeather = wd;
      }
    }
    http.end();
  }
}

// Draw everything on OLED
void drawOLED(){
  display.clearDisplay();

  // Waktu & Tanggal
  time_t nowTime = time(nullptr);
  struct tm * timeinfo = localtime(&nowTime);
  char buf[20];
  strftime(buf, sizeof(buf), "%d/%m %H:%M", timeinfo);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print(buf);

  // Cuaca Tangerang
  display.setCursor(0,12);
  display.setTextSize(2);
  display.print(currentWeather.tempNow,0);
  display.print("° ");

  display.setTextSize(1);
  display.setCursor(64,12);
  display.print(currentWeather.description);

  // High / Low
  display.setCursor(0,36);
  display.print("H:");
  display.print(currentWeather.tempHigh,0);
  display.print("° L:");
  display.print(currentWeather.tempLow,0);
  display.print("°");

  // Indoor Temp
  display.setCursor(0,50);
  display.print("Indoor: ");
  display.print(indoorTemp,0);
  display.print("°");

  // Battery
  float bat = readBattery();
  display.setCursor(80,50);
  display.print("Bat: ");
  display.print((int)bat);
  display.print("%");

  display.display();
}
