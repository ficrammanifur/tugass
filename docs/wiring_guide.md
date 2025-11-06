# Wiring Guide for ESP32 Mini Weather Station

## 📋 Overview
This guide provides step-by-step wiring instructions for assembling the ESP32 Mini Weather Station. The setup is simple, using I2C for the OLED display and a single GPIO for the DHT22 sensor. Ensure all connections are secure to avoid loose wires or short circuits.

### Required Tools
- Soldering iron (optional for breadboard prototyping)
- Jumper wires (male-to-female for breadboard)
- Breadboard or perfboard
- Multimeter (for continuity testing)
- ESP32 DevKit V1 or similar

### Safety Notes
- **Power Supply:** Use 3.3V for sensors and display. Do not exceed 5V on ESP32 pins.
- **ESD Protection:** Ground yourself to prevent static damage to components.
- **Testing:** Power on without sensors first to verify ESP32 boots.

---

## 🛠️ Pinout Reference
### ESP32 DevKit V1 Pin Assignments
| ESP32 Pin | Function | Connected To | Notes |
|-----------|----------|--------------|-------|
| GPIO 3 | DHT22 Data | DHT22 Data Pin | Single-wire protocol |
| GPIO 8 | I2C SDA | OLED SDA | Pull-up resistor recommended (4.7kΩ) |
| GPIO 9 | I2C SCL | OLED SCL | Pull-up resistor recommended (4.7kΩ) |
| 3.3V | Power | OLED VCC, DHT22 VCC | Shared power rail |
| GND | Ground | OLED GND, DHT22 GND | Common ground |
| 5V (optional) | USB Power | ESP32 VIN | For powering via USB |

### Component Pinouts
#### SSD1306 OLED (I2C)
- VCC → ESP32 3.3V
- GND → ESP32 GND
- SDA → ESP32 GPIO 8
- SCL → ESP32 GPIO 9

#### DHT22 Sensor
- VCC → ESP32 3.3V
- GND → ESP32 GND
- Data → ESP32 GPIO 3
- (NC) → Not Connected

---

## 🔌 Step-by-Step Wiring Instructions

### 1. Prepare the Breadboard
- Place ESP32 on one side of the breadboard.
- Use power rails for 3.3V and GND distribution.

### 2. Wire the OLED Display
1. Connect OLED VCC to ESP32 3.3V rail.
2. Connect OLED GND to ESP32 GND rail.
3. Connect OLED SDA to ESP32 GPIO 8 (use jumper wire).
4. Connect OLED SCL to ESP32 GPIO 9 (use jumper wire).
5. **Optional:** Add 4.7kΩ pull-up resistors between SDA/SCL and 3.3V for stable I2C.

**Visual (ASCII Art):**
```
ESP32          OLED
-----         -----
3.3V  ──────── VCC
GND   ──────── GND
GPIO8 ──────── SDA
GPIO9 ──────── SCL
```

### 3. Wire the DHT22 Sensor
1. Connect DHT22 VCC to ESP32 3.3V rail.
2. Connect DHT22 GND to ESP32 GND rail.
3. Connect DHT22 Data to ESP32 GPIO 3 (use jumper wire).
4. **Optional:** Add a 10kΩ pull-up resistor between Data and VCC for reliability.

**Visual (ASCII Art):**
```
ESP32          DHT22
-----         -----
3.3V  ──────── VCC
GND   ──────── GND
GPIO3 ──────── Data
```

### 4. Power Connections
- Connect ESP32 via USB for initial testing (provides 5V to VIN).
- For standalone: Use external 5V supply to VIN and GND.
- Verify with multimeter: 3.3V rail stable at ~3.3V.

### 5. Full Assembly Diagram
(Refer to `/assets/Schematic-Weather-Station.png` for visual schematic.)

**Breadboard Layout Suggestion:**
- Left rail: ESP32
- Center: OLED (top) + DHT22 (bottom)
- Right rail: Power distribution

---

## 🔍 Verification Steps
1. **Continuity Test:** Use multimeter to check connections (beep on shorts).
2. **Power Test:** Measure voltages: 3.3V rail = 3.3V, no shorts to GND.
3. **Upload Test Code:** Use `test/oled_test.ino` – should display "Hello OLED".
4. **DHT Test:** Use `test/dht_test.ino` – serial output shows temperature.
5. **Full Test:** Upload main firmware; verify slides and sensor readings.

### Common Issues & Fixes
| Issue | Cause | Fix |
|-------|-------|-----|
| OLED Blank | Loose I2C wires | Re-seat jumpers, check pull-ups |
| DHT NaN | Timing violation | Add delay >2s between reads |
| I2C Errors | Address conflict | Scan I2C (address 0x3C for OLED) |
| Power Drop | Long wires | Shorten jumpers, add capacitors |

---

## 📝 References
- [Adafruit SSD1306 Guide](https://learn.adafruit.com/monochrome-oled-breakouts/wiring-128x64-oleds)
- [DHT22 with ESP32](https://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-sensor-arduino-ide/)
- [ESP32 I2C Pins](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html)

*Last Updated: November 06, 2025*

---

# FreeRTOS Guide for ESP32 Mini Weather Station

## 📋 Overview
Although the core firmware uses the Arduino framework (which wraps FreeRTOS under the hood), this guide explains FreeRTOS concepts for advanced customization. Arduino's `loop()` runs as a FreeRTOS task, but you can create additional tasks for concurrent operations like sensor reading or API fetching. This is useful for scaling the project (e.g., adding more sensors).

**Note:** The provided `main.ino` does not explicitly use FreeRTOS APIs. To enable multi-tasking, include `<freertos/FreeRTOS.h>` and `<freertos/task.h>`.

### Why FreeRTOS on ESP32?
- **Preemptive Multi-Tasking:** Run sensor reads, display updates, and WiFi fetches concurrently.
- **Priorities:** High-priority task for real-time display (50ms), low for API (15min).
- **Queues:** Pass data (e.g., temperature) between tasks safely.
- **Arduino Compatibility:** Arduino's `setup()`/`loop()` are FreeRTOS tasks by default.

---

## 🛠️ FreeRTOS Basics in Arduino-ESP32
### Core Concepts
| Concept | Description | Use in Project |
|---------|-------------|----------------|
| **Task** | Independent function running concurrently | Display update task, DHT read task |
| **Priority** | 0 (lowest) to 24 (highest) | Display: 5, API: 1 |
| **Stack Size** | Memory per task (bytes) | 2048-4096 for simple tasks |
| **Queue** | FIFO buffer for inter-task communication | Send weather data from API task to display |
| **Mutex/Semaphore** | Synchronization primitives | Protect shared OLED access |

### Task States
- **Ready:** Waiting for CPU.
- **Running:** Executing on core.
- **Blocked:** Waiting on queue/semaphore.
- **Suspended:** Paused manually.

---

## 🔧 Creating Tasks
### Example: Separate DHT Task
Add to `main.ino` after includes:
```cpp
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Global queue for temperature
QueueHandle_t tempQueue;

// DHT Task Function
void dhtTask(void *pvParameters) {
  while (1) {
    float t = dht.readTemperature();
    if (!isnan(t)) {
      xQueueSend(tempQueue, &t, portMAX_DELAY);  // Send to queue
    }
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 sec delay
  }
}

// In setup():
tempQueue = xQueueCreate(5, sizeof(float));  // Queue size 5
xTaskCreate(dhtTask, "DHT_Task", 2048, NULL, 2, NULL);  // Priority 2

// In loop(): Receive from queue
float roomTempFloat;
if (xQueueReceive(tempQueue, &roomTempFloat, pdMS_TO_TICKS(100)) == pdTRUE) {
  roomTemp = String((int)round(roomTempFloat));
}
```

### Task Creation API
```cpp
xTaskCreate(
  TaskFunction_t pvTaskCode,    // Function pointer
  const char * const pcName,    // Task name
  const uint32_t usStackDepth,  // Stack size (words, ~4 bytes each)
  void * const pvParameters,    // Param to task
  UBaseType_t uxPriority,       // Priority 0-24
  TaskHandle_t * const pxCreatedTask  // Handle (optional)
);
```

**Best Practices:**
- Stack: 1024-4096 words (test with `uxTaskGetStackHighWaterMark()`).
- Priority: Avoid >10 to prevent starvation.
- Delete: `vTaskDelete(NULL)` at end of task (rarely needed).

---

## 📨 Queues & Communication
### Queue Example: Weather Data
```cpp
// Define struct for weather data
typedef struct {
  String suhu;
  String cuacaSekarang;
  int uv;
} WeatherData_t;

QueueHandle_t weatherQueue = xQueueCreate(1, sizeof(WeatherData_t));  // Size 1

// Sender Task (API fetch)
void weatherTask(void *pvParameters) {
  while (1) {
    // Fetch logic...
    WeatherData_t data = {suhu, cuacaSekarang, uvIndex};
    xQueueSend(weatherQueue, &data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(900000));  // 15 min
  }
}

// Receiver in loop()
WeatherData_t receivedData;
if (xQueueReceive(weatherQueue, &receivedData, 0) == pdTRUE) {
  suhu = receivedData.suhu;
  // Update display
}
```

### Queue API
- **Create:** `xQueueCreate(uxQueueLength, uxItemSize)`
- **Send:** `xQueueSend(queue, &item, ticksToWait)` (blocking)
- **Receive:** `xQueueReceive(queue, &item, ticksToWait)`
- **Peek:** Non-destructive receive.

**Tips:** Use `portMAX_DELAY` for blocking; size queue to buffer bursts.

---

## 🔒 Synchronization
### Mutex for Shared Resources (e.g., OLED)
```cpp
SemaphoreHandle_t oledMutex = xSemaphoreCreateMutex();

// In display task
if (xSemaphoreTake(oledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
  display.clearDisplay();
  // Draw...
  display.display();
  xSemaphoreGive(oledMutex);
}
```

### Binary Semaphore for Events
```cpp
SemaphoreHandle_t updateSemaphore = xSemaphoreCreateBinary();

// Notify from API task
xSemaphoreGive(updateSemaphore);

// Wait in display task
xSemaphoreTake(updateSemaphore, portMAX_DELAY);
```

---

## ⚡ Performance Tuning
### Task Monitoring
```cpp
// In loop(), print stats
UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
Serial.printf("Stack free: %d words\n", uxHighWaterMark);

// List all tasks
vTaskList();  // Prints to Serial
```

### Common Pitfalls
| Issue | Symptom | Fix |
|-------|---------|-----|
| **Stack Overflow** | Crashes/random resets | Increase `usStackDepth` |
| **Priority Inversion** | High-priority task starves | Use mutex with inheritance |
| **Deadlock** | Tasks hang | Avoid nested locks |
| **Queue Full** | Data loss | Increase queue size or use overwrite |

### Core Affinity
Pin tasks to cores:
```cpp
xTaskCreatePinnedToCore(taskFunc, "Task", 2048, NULL, 1, NULL, 0);  // Core 0
```

---

## 📝 References
- [ESP-IDF FreeRTOS Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)
- [Arduino-ESP32 FreeRTOS Integration](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/Arduino.h)
- [Random Nerd Tutorials: FreeRTOS on ESP32](https://randomnerdtutorials.com/esp32-freertos-tasks-arduino-ide/)

*Last Updated: November 06, 2025*
