#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"

// WiFi credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

// DHT22
#define DHTPIN 4       // Pin data DHT22
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Open-Meteo URL Tangerang
const char* weatherURL = "https://api.open-meteo.com/v1/forecast?latitude=-6.2&longitude=106.6&hourly=temperature_2m,weathercode&timezone=Asia/Jakarta";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  dht.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
}

void loop() {
  float indoorTemp = dht.readTemperature();
  float outdoorTemp = 0;
  float highTemp = 0;
  float lowTemp = 0;
  String weatherDesc = "Unknown";

  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(weatherURL);
    int httpCode = http.GET();

    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      DynamicJsonDocument doc(8192);
      deserializeJson(doc, payload);

      // Ambil suhu sekarang (index 0)
      outdoorTemp = doc["hourly"]["temperature_2m"][0].as<float>();

      // Ambil max & min (misal dari 24 jam pertama)
      highTemp = doc["hourly"]["temperature_2m"][0].as<float>();
      lowTemp = doc["hourly"]["temperature_2m"][0].as<float>();
      for(int i=0;i<24;i++){
        float t = doc["hourly"]["temperature_2m"][i].as<float>();
        if(t>highTemp) highTemp = t;
        if(t<lowTemp) lowTemp = t;
      }

      // Cuaca sederhana (berdasarkan kode)
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

  // Tampilkan ke OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0,0);
  display.println("Tangerang Weather");
  
  display.setTextSize(2);
  display.setCursor(0,16);
  display.print((int)outdoorTemp); display.print("° ");
  display.setTextSize(1);
  display.println(weatherDesc);

  display.setCursor(0, 40);
  display.print("H:"); display.print((int)highTemp); display.print("° ");
  display.print("L:"); display.print((int)lowTemp); display.print("°");

  display.setCursor(0, 52);
  display.print("Indoor: "); display.print((int)indoorTemp); display.println("°");

  display.display();

  delay(60000); // Update setiap 60 detik
}
