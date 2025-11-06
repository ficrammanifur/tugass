//buka di https://wokwi.com/projects/445986662366934017
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <time.h>  // Untuk localtime dan strftime
#include <esp_sleep.h>  // Untuk sleep mode
#include "driver/gpio.h"  // Untuk PIR interrupt

// ---------------- USER CONFIG ----------------
const char* ssid = "Wokwi-GUEST"; // Ganti dengan SSID WiFi Anda
const char* password = ""; // Ganti dengan password WiFi Anda
const char* OPENWEATHER_API_KEY = "55d2233806228fa3fc5b5be287949c50"; // <-- GANTI DENGAN KEY BARU DARI openweathermap.org

const char* WEATHER_CITY = "Tangerang,ID";  // Kota untuk OpenWeather

// DHT config
#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// PIR config
#define PIR_PIN 19
volatile bool pirDetected = false;  // Flag untuk interrupt PIR
const unsigned long PIR_TIMEOUT = 30000;  // 30 detik tanpa deteksi → sleep

// OLED config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Parameter mata (sama seperti kode sederhana Anda)
int leftEyeCenterX = 40;
int eyeCenterY = 22;
int rightEyeCenterX = 88;
int eyeRadius = 12;
int posList[8] = {-4, -2, 0, 2, 4, 2, 0, -2};

// Timing
const unsigned long eyesDuration = 3000;  // Animasi mata 3s
const unsigned long infoDuration = 4000;  // Info screen 4s
const unsigned long dhtInterval = 10000;  // DHT update 10s
const unsigned long weatherInterval = 600000;  // Weather update 10 menit

// Variabel global untuk data sensor (dideklarasikan di atas agar accessible semua task)
float weatherTemp = NAN;
String weatherMain = "--";
float roomTemp = NAN;
float roomHum = NAN;
unsigned long lastDhtMillis = 0;
unsigned long lastWeatherMillis = 0;
unsigned long lastPirMillis = 0;  // Untuk timeout PIR
unsigned long modeStart = 0;
int mode = 0;  // 0: eyes, 1: info

// FreeRTOS Queue untuk komunikasi antar task (queue untuk data sensor/cuaca)
QueueHandle_t sensorQueue;  // Queue untuk struct data sensor
typedef struct {
  float weatherTemp;
  String weatherMain;
  float roomTemp;
  float roomHum;
  time_t epochTime;
} SensorData_t;

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7*3600, 60000);  // +7 timezone

// Manajemen memori: Fungsi untuk log heap dan stack
void logMemory(const char* taskName) {
  Serial.printf("[%s] Free Heap: %d bytes\n", taskName, ESP.getFreeHeap());
  // Stack high water mark bisa diakses per task dengan uxTaskGetStackHighWaterMark()
}

// Interrupt handler untuk PIR (disesuaikan signature: void* arg)
void IRAM_ATTR pirISR(void* arg) {
  pirDetected = true;
  lastPirMillis = millis();
}

// Task 1: Main Monitor Task (scheduling utama, handle sleep)
void mainMonitorTask(void *pvParameters) {
  // Setup PIR interrupt
  pinMode(PIR_PIN, INPUT);
  gpio_install_isr_service(0);
  gpio_isr_handler_add((gpio_num_t)PIR_PIN, pirISR, NULL);

  // Create queue
  sensorQueue = xQueueCreate(5, sizeof(SensorData_t));  // Queue ukuran 5 items

  // Connect WiFi sekali di awal
  wifiConnect();

  // Init NTP
  timeClient.begin();
  timeClient.update();

  // Initial read
  readDHT();
  fetchWeather();

  // Kirim initial data ke queue
  SensorData_t initialData;
  initialData.weatherTemp = NAN;
  initialData.weatherMain = "--";
  initialData.roomTemp = roomTemp;
  initialData.roomHum = roomHum;
  initialData.epochTime = timeClient.getEpochTime();
  xQueueSend(sensorQueue, &initialData, portMAX_DELAY);

  unsigned long lastActivity = millis();
  for (;;) {
    unsigned long now = millis();

    // Cek PIR
    if (pirDetected) {
      pirDetected = false;
      lastActivity = now;  // Reset timeout
      Serial.println("PIR detected - Wake up!");
      // Kirim sinyal ke display task via queue atau event group (sederhana: set flag global atau restart task)
    }

    // Jika tidak ada aktivitas PIR dalam timeout, sleep
    if (now - lastActivity > PIR_TIMEOUT) {
      Serial.println("No motion - Entering light sleep...");
      logMemory("MainMonitor");
      esp_sleep_enable_ext0_wakeup((gpio_num_t)PIR_PIN, 1);  // Wake on PIR HIGH
      esp_light_sleep_start();  // Light sleep (wake on interrupt)
      lastActivity = millis();  // Reset setelah wake
      Serial.println("Woke up from sleep!");
    }

    // Update NTP periodically
    timeClient.update();

    vTaskDelay(pdMS_TO_TICKS(1000));  // Delay 1s (scheduling)
  }
}

// Task 2: Sensor Update Task (update DHT & Weather, kirim ke queue)
void sensorUpdateTask(void *pvParameters) {
  for (;;) {
    unsigned long now = millis();

    // Update DHT
    if (now - lastDhtMillis >= dhtInterval) {
      lastDhtMillis = now;
      readDHT();
    }

    // Update weather
    if (now - lastWeatherMillis >= weatherInterval) {
      lastWeatherMillis = now;
      fetchWeather();
    }

    // Pack data dan kirim ke queue
    SensorData_t data;
    data.weatherTemp = weatherTemp;
    data.weatherMain = weatherMain;
    data.roomTemp = roomTemp;
    data.roomHum = roomHum;
    data.epochTime = timeClient.getEpochTime();
    if (xQueueSend(sensorQueue, &data, pdMS_TO_TICKS(100)) != pdTRUE) {
      Serial.println("Queue full - data dropped!");
    }

    logMemory("SensorUpdate");  // Log memori

    vTaskDelay(pdMS_TO_TICKS(5000));  // Delay 5s
  }
}

// Task 3: Display Task (handle animasi mata dan info, baca dari queue)
void displayTask(void *pvParameters) {
  SensorData_t currentData;
  for (;;) {
    // Baca dari queue (blocking jika kosong)
    if (xQueueReceive(sensorQueue, &currentData, pdMS_TO_TICKS(1000)) == pdTRUE) {
      // Update global vars dari queue (untuk display)
      weatherTemp = currentData.weatherTemp;
      weatherMain = currentData.weatherMain;
      roomTemp = currentData.roomTemp;
      roomHum = currentData.roomHum;
    }

    unsigned long now = millis();

    // Mode bergantian (hanya jika wake/active)
    if (mode == 0) {  // Eyes mode
      if (now - modeStart < eyesDuration) {
        runEyesAnimation();
      } else {
        mode = 1;
        modeStart = now;
        display.clearDisplay();
        display.display();
      }
    } else {  // Info mode
      if (now - modeStart < infoDuration) {
        showInfoScreen(currentData.epochTime);  // Pass epoch untuk waktu
      } else {
        mode = 0;
        modeStart = now;
        display.clearDisplay();
        display.display();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));  // Scheduling delay
  }
}

// ---------------- Animasi Mata (sederhana seperti kode Anda) ----------------
void runEyesAnimation() {
  static int posIndex = 0;
  static unsigned long lastFrame = 0;
  unsigned long now = millis();

  // Gerakan swaying (loop posList)
  if (now - lastFrame >= 150) {  // 0.15s per frame
    drawEyes(posList[posIndex], posList[posIndex], false);
    posIndex = (posIndex + 1) % 8;
    lastFrame = now;
  }

  // Blink random (setiap ~2-3s)
  static unsigned long lastBlink = 0;
  if (now - lastBlink > 2000 && random(0, 100) < 5) {  // 5% chance
    drawEyes(0, 0, true);
    vTaskDelay(pdMS_TO_TICKS(200));
    drawEyes(0, 0, false);
    lastBlink = now;
  }
}

// Gambar Mata (sama persis seperti kode Anda)
void drawEyes(int leftOffset, int rightOffset, bool blink) {
  display.clearDisplay();

  if (blink) {
    for (int w = 0; w < 2; w++) {
      display.drawLine(28 + leftOffset, 22 + w, 52 + leftOffset, 22 + w, SSD1306_WHITE);
      display.drawLine(76 + rightOffset, 22 + w, 100 + rightOffset, 22 + w, SSD1306_WHITE);
    }
  } else {
    int lx = leftEyeCenterX + leftOffset;
    display.fillCircle(lx, eyeCenterY, eyeRadius, SSD1306_WHITE);
    display.drawCircle(lx, eyeCenterY, eyeRadius, SSD1306_WHITE);

    int rx = rightEyeCenterX + rightOffset;
    display.fillCircle(rx, eyeCenterY, eyeRadius, SSD1306_WHITE);
    display.drawCircle(rx, eyeCenterY, eyeRadius, SSD1306_WHITE);
  }

  display.display();
}

// ---------------- Info Screen (modif untuk pass epoch) ----------------
void showInfoScreen(time_t epoch) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Waktu NTP
  if (epoch > 0) {
    struct tm *ti = localtime(&epoch);
    char timebuf[20];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", ti);
    char datebuf[20];
    strftime(datebuf, sizeof(datebuf), "%d %b %Y", ti);
    display.setCursor(0, 0);
    display.printf("%s  %s", timebuf, datebuf);
  } else {
    display.setCursor(0, 0);
    display.print("Time: Syncing...");
  }

  // Cuaca OpenWeather
  display.setCursor(0, 12);
  display.print("Tangerang, ID");
  display.setCursor(0, 24);
  display.setTextSize(2);
  if (!isnan(weatherTemp)) {
    display.printf("%.0f C", weatherTemp);
  } else {
    display.print("-- C");
  }
  display.setCursor(70, 24);
  display.setTextSize(1);
  display.print(weatherMain);

  // DHT
  display.setCursor(0, 50);
  display.setTextSize(1);
  if (!isnan(roomTemp) && !isnan(roomHum)) {
    display.printf("Room: %.1fC  %.0f%%", roomTemp, roomHum);
  } else {
    display.print("Room: --C  --%");
  }

  display.display();
}

// ---------------- Helpers (sama seperti sebelumnya) ----------------
void wifiConnect() {
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 15000) {
      Serial.println("\nWiFi failed. Reboot...");
      ESP.restart();
    }
  }
  Serial.println("\nWiFi OK! IP: " + WiFi.localIP().toString());
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(WEATHER_CITY) +
               "&units=metric&appid=" + String(OPENWEATHER_API_KEY);
  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, payload);
    weatherTemp = doc["main"]["temp"];
    weatherMain = doc["weather"][0]["main"].as<String>();
    Serial.printf("Weather: %.0fC %s\n", weatherTemp, weatherMain.c_str());
  } else {
    Serial.printf("Weather error: %d\n", code);
    weatherTemp = NAN;
    weatherMain = "Error";
  }
  http.end();
}

void readDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    roomTemp = t;
    roomHum = h;
    Serial.printf("DHT: %.1fC %.0f%%\n", t, h);
  } else {
    Serial.println("DHT failed - cek koneksi pin 4");
    roomTemp = NAN;
    roomHum = NAN;
  }
}

// ----- setup -----
void setup() {
  Serial.begin(115200);
  delay(100);

  // Init OLED
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 failed");
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("FreeRTOS: Mata + Cuaca + DHT + PIR");
  display.display();
  delay(2000);

  // Init DHT
  dht.begin();

  // Buat tasks dengan stack size terdefinisi (manajemen memori)
  xTaskCreatePinnedToCore(
    mainMonitorTask,   // Task function
    "MainMonitor",     // Task name
    4096,              // Stack size (bytes) - heap/stack management
    NULL,              // Parameters
    1,                 // Priority
    NULL,              // Task handle
    0                  // Core 0
  );

  xTaskCreatePinnedToCore(
    sensorUpdateTask,  // Task function
    "SensorUpdate",    // Task name
    4096,              // Stack size
    NULL,
    1,
    NULL,
    1                  // Core 1
  );

  xTaskCreatePinnedToCore(
    displayTask,       // Task function
    "Display",         // Task name
    8192,              // Stack size lebih besar untuk GFX
    NULL,
    2,                 // Higher priority untuk display
    NULL,
    1                  // Core 1
  );

  // Hapus setup task (FreeRTOS auto)
  vTaskDelete(NULL);
}

// Tambahkan fungsi loop() kosong untuk Arduino framework (hindari linker error)
void loop() {
  // Semua logic di-handle oleh FreeRTOS tasks, loop() idle saja
  vTaskDelay(pdMS_TO_TICKS(1000));
}
