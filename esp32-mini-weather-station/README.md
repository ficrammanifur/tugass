<h1 align="center">
🕐 ESP32 Portable Digital Clock<br>
    <sub>OLED Display with Animated Mochi Eyes & Deep Sleep </sub>
</h1>

<p align="center">
  <img src="/assets/portable_clock_banner.png?height=400&width=700" alt="ESP32 Portable Clock" width="700"/>
</p>
<p align="center">
  <em>Jam digital portable berbasis ESP32-C3 dengan tampilan OLED 128x64, animasi mata mochi lucu, sinkronisasi waktu NTP real-time, sensor DHT22 untuk suhu ruangan, FreeRTOS tasks untuk scheduling, monitoring memory, dan deep sleep untuk hemat daya hingga <1mA.</em>
</p>
      
<p align="center">
  <img src="https://img.shields.io/badge/last_commit-today-brightgreen?style=for-the-badge" />
  <img src="https://img.shields.io/badge/language-C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" />
  <img src="https://img.shields.io/badge/platform-ESP32--C3_▸_OLED-00ADD8?style=for-the-badge&logo=espressif&logoColor=white" />
  <img src="https://img.shields.io/badge/framework-Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white" />
  <img src="https://img.shields.io/badge/RTOS-FreeRTOS-3C873A?style=for-the-badge&logo=freebsd&logoColor=white" />
  <img src="https://img.shields.io/badge/sensors-DHT22-32CD32?style=for-the-badge&logo=sensors&logoColor=white" />
  <img src="https://img.shields.io/badge/API-NTP-7B68EE?style=for-the-badge&logo=clock&logoColor=white" />
  <a href="https://github.com/ficrammanifur/esp32-portable-digital-clock/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/license-MIT-blue?style=for-the-badge" alt="License: MIT" />
  </a>
</p>

---

## 📋 Daftar Isi
- [Desain Lengkap](#-desain-lengkap)
  - [Desain Hardware](#-desain-hardware)
  - [Desain Software](#-desain-software)
- [Penjelasan Program](#-penjelasan-program)
  - [Arsitektur FreeRTOS](#-arsitektur-freertos)
  - [🏗️ Arsitektur Sistem](#-arsitektur-sistem)
- [Instalasi](#-instalasi)
- [Cara Menjalankan](#-cara-menjalankan)
- [Testing](#-testing)
- [Troubleshooting](#-troubleshooting)
- [Struktur Folder](#-struktur-folder)
- [Kontribusi](#-kontribusi)
- [Pengembang](#-pengembang)
- [Lisensi](#-lisensi)

---

## 📐 Desain Lengkap

### Desain Hardware
Project ini dirancang sebagai jam digital portable yang kompak, battery-powered, dengan dimensi ~5x5cm (casing 3D-printable). Fokus pada low-power consumption (<1mA saat sleep) untuk bertahan 1-2 minggu pada battery LiPo 3.7V 500mAh.

#### Komponen Utama
| Komponen | Fungsi | Spesifikasi | Keterangan |
|----------|--------|-------------|------------|
| **ESP32-C3 DevKit** | Mikrokontroler utama | RISC-V 160MHz, 400KB SRAM, WiFi | Handle FreeRTOS tasks, NTP sync, deep sleep |
| **SSD1306 OLED 128x64** | Tampilan | I2C, 3.3V, 0.96" | 4 slides: Eyes, Time, Date, Battery |
| **DHT22** | Sensor suhu/kelembapan | GPIO 2, 1-wire | Pembacaan suhu dan kelembapan ruangan |
| **LiPo Battery 3.7V 500mAh** | Power source | Dengan TP4056 charger | Portable; monitor via ADC GPIO 0 |
| **Resistor Divider** | Battery sensing | 10kΩ + 4.7kΩ | Scale 3.7-4.2V ke 0-3.3V untuk ADC |
| **Push Button** | Manual reset | Momentary switch | Optional untuk force reboot |
| **3D-Printed Case** | Enclosure | PLA/ABS, 5x5x1.5cm | Slot untuk OLED, USB charging |

#### Diagram Blok Hardware
```mermaid
graph TD
    A[LiPo 3.7V Battery + TP4056 Charger] -->|3.3V Regulated| B[ESP32-C3 DevKit]
    B -->|I2C: GPIO 8 SDA, GPIO 9 SCL| C[SSD1306 OLED 128x64]
    B -->|GPIO 2| D[DHT22 Sensor - Suhu/Kelembapan]
    B -->|GPIO 0: ADC| E[Resistor Divider - Battery Sensing]
    C -->|Display Slides & Low Power Off| F[User Interface]
    D -->|1-wire Protocol| G[Sensor Data]
    style A fill:#f9f,stroke:#333,stroke-width:2px
    style B fill:#bbf,stroke:#333,stroke-width:2px
    style C fill:#ff9,stroke:#333,stroke-width:2px
    style D fill:#9f9,stroke:#333,stroke-width:2px
```

#### Wiring Diagram
<p align="center">
  <img src="/assets/portable_clock_wiring.png?height=400&width=700" alt="ESP32 Portable Clock Wiring Diagram" width="700"/><br/>
  <em>Diagram Pengkabelan Lengkap</em><br/>
  ⚙️ <strong>Notes:</strong><br/>
  🔹 Battery: + ke TP4056 IN+, - ke GND; OUT+ ke VIN ESP32-C3 (atau 3.3V reg).
  🔹 ADC Battery: GPIO 0 (analogRead) dengan divider untuk aman <3.3V.
  🔹 DHT22: Data ke GPIO 2 (1-wire protocol).
  🔹 Charging: USB Micro-B via TP4056 untuk portable.
  🔹 Total Power: Active ~50mA, Sleep <1mA.
</p>

#### Desain Casing (3D Printable)
- **Ukuran**: 50x50x15mm (compact untuk saku/meja).
- **Fitur**: Lubang OLED transparan, DHT22 exposed, USB port akses, slot battery internal.
- **File STL**: Tersedia di `/assets/case.stl` (desain via Tinkercad).
- **Biaya Estimasi**: ~Rp 150.000 (ESP32-C3 Rp50k, OLED Rp30k, Battery Rp40k, Lainnya Rp30k).

### Desain Software
- **Framework**: Arduino IDE + FreeRTOS (built-in ESP32).
- **Fitur Utama**:
  - **NTP Sync**: Sinkronisasi waktu Asia/Jakarta setiap 1 jam.
  - **Display Slides**: 4 layar (Eyes animasi, Jam besar, Tanggal, Battery level).
  - **Animasi Mochi Eyes**: Blink random + offset gerak.
  - **Deep Sleep**: Auto setelah 10 menit; wake via timer atau manual reset.
  - **Battery Monitor**: ADC read, tampil % & voltage di slide.
  - **FreeRTOS**: Tasks terpisah untuk non-blocking operation.
- **Memory Management**: ESP.getFreeHeap() & uxTaskGetStackHighWaterMark() di MonitorTask.
- **Queue**: xQueue untuk share time/battery/sensor data antar tasks.
- **Fallback**: Jika NTP gagal, gunakan RTC internal (RTC_DATA_ATTR).

#### Arsitektur Software - FreeRTOS Architecture
```mermaid
graph TB
    A[FreeRTOS Scheduler] --> B[DisplayTask<br/>Priority 1, Stack 8192<br/>Update OLED: Animasi, Slides]
    A --> C[TimeTask<br/>Priority 2, Stack 4096<br/>NTP Sync → Queue TimeData]
    A --> D[SensorTask<br/>Priority 3, Stack 2048<br/>DHT22 Read → Queue SensorData]
    A --> E[BatteryTask<br/>Priority 4, Stack 2048<br/>ADC Read → Queue BatteryData]
    A --> F[MonitorTask<br/>Priority 5, Stack 2048<br/>Memory Monitor & Deep Sleep]
    B -->|Dequeue from Queues| G[OLED Display]
    C -->|"xQueueCreate(5, sizeof(TimeData))"| H[Queue: TimeData]
    D -->|"xQueueCreate(5, sizeof(SensorData))"| I[Queue: SensorData]
    E -->|"xQueueCreate(5, sizeof(BatteryData))"| J[Queue: BatteryData]
    F -->|"ESP.getFreeHeap() & uxTaskGetStackHighWaterMark()"| K[Serial Log]
    style A fill:#ff9,stroke:#333,stroke-width:2px
    style B fill:#9f9,stroke:#333,stroke-width:2px
    style C fill:#9f9,stroke:#333,stroke-width:2px
    style D fill:#9f9,stroke:#333,stroke-width:2px
    style E fill:#9f9,stroke:#333,stroke-width:2px
    style F fill:#f99,stroke:#333,stroke-width:2px
```

---

## 💻 Penjelasan Program

### Arsitektur FreeRTOS
Program menggunakan FreeRTOS untuk multitasking efisien, menghindari blocking delays di loop utama. Setiap task punya prioritas, stack size, dan vTaskDelay untuk scheduling.
- **Queue**: `xQueueCreate(5, sizeof(TimeData))` untuk share struct {int hour, min, day; ...} dari TimeTask ke DisplayTask. Serupa untuk SensorData dan BatteryData.
- **Tasks**:
  - **DisplayTask**: Core UI, dequeue data, update OLED setiap 50ms. Handle slide cycle & eye animation.
  - **TimeTask**: Sync NTP via configTime & getLocalTime, kirim ke queue setiap 1 jam atau on-wake.
  - **SensorTask**: Baca DHT22 (suhu/kelembapan), kirim ke queue setiap 2 detik.
  - **BatteryTask**: Baca ADC (GPIO 0), hitung voltage/%, kirim ke queue setiap 30 detik.
  - **MonitorTask**: Cek memory, log heap, trigger deep sleep jika >10min inactivity.
- **Deep Sleep Integration**: esp_deep_sleep_start() di MonitorTask. On-wake, resume tasks seamlessly via RTC_DATA_ATTR.
- **Memory**: MonitorTask print "Free Heap: X | Stack WM: Y" setiap 10s via Serial.
- **Portability**: RTC_DATA_ATTR untuk persist time/battery kalibrasi saat sleep/reboot.

### 🏗️ Arsitektur Sistem

#### Diagram Alur Data (Data Flow Diagram)
```mermaid
flowchart TD
    A[WiFiManager Setup<br/>Captive Portal SSID/Password] -->|WiFi Connect| B[FreeRTOS Tasks<br/>xTaskCreate]
    B --> C[TimeTask<br/>1 hour interval<br/>Fetch NTP → Queue TimeData<br/>Fallback: RTC internal]
    B --> D[SensorTask<br/>2 sec interval<br/>DHT22 Read → Queue SensorData]
    B --> E[BatteryTask<br/>30 sec interval<br/>ADC GPIO 0 → Queue BatteryData]
    B --> F[DisplayTask<br/>50 ms interval<br/>Dequeue Data → Animate & Draw Slides]
    B --> G[MonitorTask<br/>1 sec interval<br/>"FreeHeap & Stack WM"<br/>Inactivity >10min → Deep Sleep]
    F -->|I2C| H[OLED Display 128x64<br/>Slide 0: Mochi Eyes + Memory<br/>Slide 1: Time & Date<br/>Slide 2: Room Temp & Humidity<br/>Slide 3: Battery Level]
    C -.->|Queue| F
    D -.->|Queue| F
    E -.->|Queue| F
    G -.->|Trigger| I["Deep Sleep<br/>esp_deep_sleep_start()"<br/>On-Wake: Resume Tasks]
    style A fill:#bbf,stroke:#333,stroke-width:2px
    style B fill:#ff9,stroke:#333,stroke-width:2px
    style H fill:#ff9,stroke:#333,stroke-width:2px
    style I fill:#f99,stroke:#333,stroke-width:2px
```

---

## ⚙️ Instalasi
1. **Clone Repo**: `git clone https://github.com/ficrammanifur/esp32-portable-digital-clock.git`
2. **Arduino IDE**: Install ESP32 package, libraries (Adafruit SSD1306/GFX, WiFiManager, DHT library).
3. **Edit Pins**: Sesuaikan GPIO pins jika berbeda dari design.
4. **Upload**: Board: ESP32C3 Dev Module → Upload → Monitor Serial 115200.

---

## 🚀 Cara Menjalankan
1. **Assembly**: Wiring seperti diagram, charge battery via USB.
2. **Power On**: Connect WiFi via hotspot "PortableClock-Setup".
3. **Test**: Lihat slides cycle, monitor suhu/baterai, tunggu 10min untuk deep sleep.
4. **Portable Mode**: Lepas USB, gunakan battery; monitor % di slide battery.

---

## 🧪 Testing
- **Time Sync**: Serial cek "Hour: XX:XX" akurat vs phone.
- **Sensor**: DHT22 read suhu/kelembapan; cross-check dengan alat manual.
- **Battery**: Analog read → Voltage 3.7-4.2V, % 0-100.
- **Sleep**: Tunggu 10min → Current <1mA.
- **Memory**: Heap >250KB, Stack WM >1KB/task.
- **Slides**: Cycle 10s, no flicker.

---

## 🐞 Troubleshooting
- **No Time Update**: Cek WiFi/NTP; fallback RTC_DATA_ATTR active.
- **Sleep Stuck**: Cek power draw; validate esp_deep_sleep setup.
- **Low Battery Read**: Calib divider resistor; analogReadResolution(12).
- **DHT22 Error**: Cek koneksi GPIO 2; verify pull-up resistor 4.7kΩ.
- **Task Crash**: Naikkan stack size; cek queue overflow.

---

## 📁 Struktur Folder
```
esp32-portable-digital-clock/
├── main.ino # FreeRTOS tasks & main logic
├── EyeAnimation.h # Animasi mata Mochi
├── assets/ # Diagrams, STL files, slide images
├── test/ # oled_test.ino, sensor_test.ino, battery_test.ino
├── docs/ # wiring.md, setup guide
├── LICENSE
└── README.md
```

---

## 🤝 Kontribusi
**Fork → Branch → Commit → PR. Ideas: Tambah alarm task, BLE sync, sensor calibration UI.**

---

## 👨‍💻 Pengembang
**Ficram Manifur Farissa**
GitHub: [@ficrammanifur](https://github.com/ficrammanifur)
Email: ficramm@gmail.com
Acknowledgments: Adafruit, Espressif, Community.

---

## 📄 Lisensi
MIT License (c) 2025 Ficram Manifur Farissa. Lihat [LICENSE](LICENSE).
<div align="center">
   
**Portable Digital Clock with FreeRTOS & Cute Animations**
**Star if helpful!**
<p><a href="#top">⬆ Top</a></p>
</div>
