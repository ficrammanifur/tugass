
---

# Panduan FreeRTOS untuk Stasiun Cuaca Mini ESP32

## 📋 Gambaran Umum
Meskipun firmware inti menggunakan framework Arduino (yang membungkus FreeRTOS di balik layar), panduan ini menjelaskan konsep FreeRTOS untuk kustomisasi lanjutan. `loop()` Arduino berjalan sebagai task FreeRTOS, tetapi Anda bisa buat task tambahan untuk operasi konkuren seperti baca sensor atau ambil API. Ini berguna untuk skala proyek (misalnya, tambah sensor lebih banyak).

**Catatan:** `main.ino` yang disediakan tidak secara eksplisit menggunakan API FreeRTOS. Untuk mengaktifkan multi-tasking, sertakan `<freertos/FreeRTOS.h>` dan `<freertos/task.h>`.

### Mengapa FreeRTOS di ESP32?
- **Multi-Tasking Preemptive:** Jalankan baca sensor, update tampilan, dan ambil WiFi secara bersamaan.
- **Prioritas:** Task prioritas tinggi untuk tampilan real-time (50ms), rendah untuk API (15 menit).
- **Antrian:** Kirim data (misalnya, suhu) antar task dengan aman.
- **Kompatibilitas Arduino:** `setup()`/`loop()` adalah task FreeRTOS secara default.

---

## 🛠️ Dasar FreeRTOS di Arduino-ESP32
### Konsep Inti
| Konsep | Deskripsi | Penggunaan di Proyek |
|--------|-----------|----------------------|
| **Task** | Fungsi independen yang berjalan bersamaan | Task update tampilan, task baca DHT |
| **Prioritas** | 0 (terendah) hingga 24 (tertinggi) | Tampilan: 5, API: 1 |
| **Ukuran Stack** | Memory per task (byte) | 2048-4096 untuk task sederhana |
| **Antrian** | Buffer FIFO untuk komunikasi antar task | Kirim data cuaca dari task API ke tampilan |
| **Mutex/Semaphore** | Primitif sinkronisasi | Lindungi akses OLED bersama |

### Status Task
- **Siap:** Menunggu CPU.
- **Berjalan:** Dieksekusi di core.
- **Diblokir:** Menunggu antrian/semaphore.
- **Dijeda:** Dihentikan manual.

---

## 🔧 Membuat Task
### Contoh: Task DHT Terpisah
Tambahkan ke `main.ino` setelah include:
```cpp
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Antrian global untuk suhu
QueueHandle_t tempQueue;

// Fungsi Task DHT
void dhtTask(void *pvParameters) {
  while (1) {
    float t = dht.readTemperature();
    if (!isnan(t)) {
      xQueueSend(tempQueue, &t, portMAX_DELAY);  // Kirim ke antrian
    }
    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay 2 detik
  }
}

// Di setup():
tempQueue = xQueueCreate(5, sizeof(float));  // Ukuran antrian 5
xTaskCreate(dhtTask, "DHT_Task", 2048, NULL, 2, NULL);  // Prioritas 2

// Di loop(): Terima dari antrian
float roomTempFloat;
if (xQueueReceive(tempQueue, &roomTempFloat, pdMS_TO_TICKS(100)) == pdTRUE) {
  roomTemp = String((int)round(roomTempFloat));
}
```

### API Pembuatan Task
```cpp
xTaskCreate(
  TaskFunction_t pvTaskCode,    // Pointer fungsi
  const char * const pcName,    // Nama task
  const uint32_t usStackDepth,  // Ukuran stack (kata, ~4 byte masing-masing)
  void * const pvParameters,    // Parameter ke task
  UBaseType_t uxPriority,       // Prioritas 0-24
  TaskHandle_t * const pxCreatedTask  // Handle (opsional)
);
```

**Praktik Terbaik:**
- Stack: 1024-4096 kata (tes dengan `uxTaskGetStackHighWaterMark()`).
- Prioritas: Hindari >10 untuk cegah kelaparan.
- Hapus: `vTaskDelete(NULL)` di akhir task (jarang diperlukan).

---

## 📨 Antrian & Komunikasi
### Contoh Antrian: Data Cuaca
```cpp
// Definisikan struct untuk data cuaca
typedef struct {
  String suhu;
  String cuacaSekarang;
  int uv;
} WeatherData_t;

QueueHandle_t weatherQueue = xQueueCreate(1, sizeof(WeatherData_t));  // Ukuran 1

// Task Pengirim (Ambil API)
void weatherTask(void *pvParameters) {
  while (1) {
    // Logika ambil...
    WeatherData_t data = {suhu, cuacaSekarang, uvIndex};
    xQueueSend(weatherQueue, &data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(900000));  // 15 menit
  }
}

// Penerima di loop()
WeatherData_t receivedData;
if (xQueueReceive(weatherQueue, &receivedData, 0) == pdTRUE) {
  suhu = receivedData.suhu;
  // Update tampilan
}
```

### API Antrian
- **Buat:** `xQueueCreate(uxQueueLength, uxItemSize)`
- **Kirim:** `xQueueSend(antrian, &item, ticksToWait)` (blocking)
- **Terima:** `xQueueReceive(antrian, &item, ticksToWait)`
- **Peek:** Terima non-destruktif.

**Tips:** Gunakan `portMAX_DELAY` untuk blocking; ukur antrian untuk buffer ledakan.

---

## 🔒 Sinkronisasi
### Mutex untuk Sumber Daya Bersama (misalnya, OLED)
```cpp
SemaphoreHandle_t oledMutex = xSemaphoreCreateMutex();

// Di task tampilan
if (xSemaphoreTake(oledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
  display.clearDisplay();
  // Gambar...
  display.display();
  xSemaphoreGive(oledMutex);
}
```

### Semaphore Biner untuk Event
```cpp
SemaphoreHandle_t updateSemaphore = xSemaphoreCreateBinary();

// Notifikasi dari task API
xSemaphoreGive(updateSemaphore);

// Tunggu di task tampilan
xSemaphoreTake(updateSemaphore, portMAX_DELAY);
```

---

## ⚡ Penyetelan Performa
### Pemantauan Task
```cpp
// Di loop(), cetak statistik
UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
Serial.printf("Stack bebas: %d kata\n", uxHighWaterMark);

// Daftar semua task
vTaskList();  // Cetak ke Serial
```

### Kesalahan Umum
| Masalah | Gejala | Perbaikan |
|---------|--------|-----------|
| **Overflow Stack** | Crash/reset acak | Tingkatkan `usStackDepth` |
| **Inversi Prioritas** | Task prioritas tinggi kelaparan | Gunakan mutex dengan inheritance |
| **Deadlock** | Task hang | Hindari kunci bersarang |
| **Antrian Penuh** | Kehilangan data | Tingkatkan ukuran antrian atau gunakan overwrite |

### Afinitas Core
Pin task ke core:
```cpp
xTaskCreatePinnedToCore(taskFunc, "Task", 2048, NULL, 1, NULL, 0);  // Core 0
```

---

## 📝 Referensi
- [Dokumentasi FreeRTOS ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)
- [Integrasi FreeRTOS Arduino-ESP32](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/Arduino.h)
- [Random Nerd Tutorials: FreeRTOS di ESP32](https://randomnerdtutorials.com/esp32-freertos-tasks-arduino-ide/)

*Terakhir Diperbarui: 06 November 2025*
