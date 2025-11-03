#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <queue>
#include <time.h>

// WiFi
const char* ssid = "SSID";
const char* password = "PASSWORD";

// DHT22
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Open-Meteo URL
const char* weatherURL = "https://api.open-meteo.com/v1/forecast?latitude=-6.2&longitude=106.6&hourly=temperature_2m,weathercode&timezone=Asia/Jakarta";

// Task intervals
unsigned long lastIndoorRead = 0;
unsigned long lastWeatherFetch = 0;
unsigned long lastDisplayUpdate = 0;

const unsigned long indoorInterval = 5000;      // 5 detik
const unsigned long weatherInterval = 60000;    // 1 menit
const unsigned long displayInterval = 1000;     // 1 detik

// Data storage (Queue)
std::queue<float> indoorTemps;
const int maxQueueSize = 10;

float outdoorTemp = 0;
float highTemp = 0;
float lowTemp = 0;
String weatherDesc = "Unknown";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("WiFi connected");

  dht.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 failed");
    for(;;);
  }
  display.clearDisplay();

  configTime(7*3600, 0, "pool.ntp.org"); // Timezone GMT+7
}

void loop() {
  unsigned long now = millis();

  // --- Task 1: Read Indoor Temp ---
  if(now - lastIndoorRead >= indoorInterval){
    lastIndoorRead = now;
    float temp = dht.readTemperature();
    if(!isnan(temp)){
      if(indoorTemps.size() >= maxQueueSize) indoorTemps.pop(); // maintain queue
      indoorTemps.push(temp);
    }
  }

  // --- Task 2: Fetch Weather ---
  if(now - lastWeatherFetch >= weatherInterval){
    lastWeatherFetch = now;
    if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      http.begin(weatherURL);
      int httpCode = http.GET();
      if(httpCode == HTTP_CODE_OK){
        String payload = http.getString();
        DynamicJsonDocument doc(8192);
        deserializeJson(doc, payload);

        // Ambil suhu sekarang (index 0)
        outdoorTemp = doc["hourly"]["temperature_2m"][0].as<float>();

        // Ambil max/min 24 jam pertama
        highTemp = lowTemp = outdoorTemp;
        for(int i=0;i<24;i++){
          float t = doc["hourly"]["temperature_2m"][i].as<float>();
          if(t>highTemp) highTemp = t;
          if(t<lowTemp) lowTemp = t;
        }

        int code = doc["hourly"]["weathercode"][0].as<int>();
        switch(code){
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

  // --- Task 3: Update OLED Display ---
  if(now - lastDisplayUpdate >= displayInterval){
    lastDisplayUpdate = now;

    // Get indoor temp latest from queue
    float indoorTemp = indoorTemps.empty() ? 0 : indoorTemps.back();

    // Get current time
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return;

    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%d/%m %H:%M", &timeinfo);

    // Display
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(timeStr);

    display.setTextSize(2);
    display.setCursor(0,12);
    display.print((int)outdoorTemp); display.print("° ");
    display.setTextSize(1);
    display.println(weatherDesc);

    display.setCursor(0,44);
    display.print("H:"); display.print((int)highTemp); display.print(" L:"); display.print((int)lowTemp);

    display.setCursor(0,54);
    display.print("Indoor: "); display.print((int)indoorTemp); display.println("°");

    display.display();
  }
}
