# Panduan Pengkabelan untuk Stasiun Cuaca Mini ESP32

## 📋 Gambaran Umum
Panduan ini memberikan instruksi pengkabelan langkah demi langkah untuk merakit Stasiun Cuaca Mini ESP32. Pengaturan sederhana, menggunakan I2C untuk tampilan OLED dan satu GPIO untuk sensor DHT22. Pastikan semua koneksi aman untuk menghindari kabel longgar atau hubungan pendek.

### Alat yang Diperlukan
- Solder (opsional untuk prototipe breadboard)
- Kabel jumper (male-to-female untuk breadboard)
- Breadboard atau perfboard
- Multimeter (untuk tes kontinuitas)
- ESP32 DevKit V1 atau serupa

### Catatan Keselamatan
- **Sumber Daya:** Gunakan 3.3V untuk sensor dan tampilan. Jangan melebihi 5V pada pin ESP32.
- **Perlindungan ESD:** Ground diri Anda untuk mencegah kerusakan statis pada komponen.
- **Testing:** Nyalakan tanpa sensor dulu untuk verifikasi boot ESP32.

---

## 🛠️ Referensi Pinout
### Penugasan Pin ESP32 DevKit V1
| Pin ESP32 | Fungsi | Terhubung Ke | Catatan |
|-----------|--------|--------------|---------|
| GPIO 3 | Data DHT22 | Pin Data DHT22 | Protokol single-wire |
| GPIO 8 | I2C SDA | OLED SDA | Resistor pull-up direkomendasikan (4.7kΩ) |
| GPIO 9 | I2C SCL | OLED SCL | Resistor pull-up direkomendasikan (4.7kΩ) |
| 3.3V | Daya | VCC OLED, VCC DHT22 | Rail daya bersama |
| GND | Ground | GND OLED, GND DHT22 | Ground umum |

### Pinout Komponen
#### SSD1306 OLED (I2C)
- VCC → Rail 3.3V ESP32
- GND → Rail GND ESP32
- SDA → GPIO 8 ESP32
- SCL → GPIO 9 ESP32

#### Sensor DHT22
- VCC → Rail 3.3V ESP32
- GND → Rail GND ESP32
- Data → GPIO 3 ESP32
- (NC) → Tidak Terhubung

---

## 🔌 Instruksi Pengkabelan Langkah demi Langkah

### 1. Siapkan Breadboard
- Tempatkan ESP32 di satu sisi breadboard.
- Gunakan rail daya untuk distribusi 3.3V dan GND.

### 2. Kabelkan Tampilan OLED
1. Hubungkan VCC OLED ke rail 3.3V ESP32.
2. Hubungkan GND OLED ke rail GND ESP32.
3. Hubungkan SDA OLED ke GPIO 8 ESP32 (gunakan kabel jumper).
4. Hubungkan SCL OLED ke GPIO 9 ESP32 (gunakan kabel jumper).
5. **Opsional:** Tambahkan resistor pull-up 4.7kΩ antara SDA/SCL dan 3.3V untuk I2C stabil.

**Visual (ASCII Art):**
```
ESP32          OLED
-----         -----
3.3V  ──────── VCC
GND   ──────── GND
GPIO8 ──────── SDA
GPIO9 ──────── SCL
```

### 3. Kabelkan Sensor DHT22
1. Hubungkan VCC DHT22 ke rail 3.3V ESP32.
2. Hubungkan GND DHT22 ke rail GND ESP32.
3. Hubungkan Data DHT22 ke GPIO 3 ESP32 (gunakan kabel jumper).
4. **Opsional:** Tambahkan resistor pull-up 10kΩ antara Data dan VCC untuk keandalan.

**Visual (ASCII Art):**
```
ESP32          DHT22
-----         -----
3.3V  ──────── VCC
GND   ──────── GND
GPIO3 ──────── Data
```

### 4. Koneksi Daya
- Hubungkan ESP32 via USB untuk testing awal (memberikan 5V ke VIN).
- Untuk standalone: Gunakan sumber eksternal 5V ke VIN dan GND.
- Verifikasi dengan multimeter: Rail 3.3V stabil ~3.3V.

### 5. Diagram Perakitan Lengkap
(Lihat `/assets/Schematic-Weather-Station.png` untuk skematik visual.)

**Saran Layout Breadboard:**
- Rail kiri: ESP32
- Tengah: OLED (atas) + DHT22 (bawah)
- Rail kanan: Distribusi daya

---

## 🔍 Langkah Verifikasi
1. **Tes Kontinuitas:** Gunakan multimeter untuk cek koneksi (beep pada hubungan pendek).
2. **Tes Daya:** Ukur voltase: Rail 3.3V = 3.3V, tanpa hubungan pendek ke GND.
3. **Upload Kode Tes:** Gunakan `test/oled_test.ino` – harus tampil "Hello OLED".
4. **Tes DHT:** Gunakan `test/dht_test.ino` – output serial tampilkan suhu.
5. **Tes Lengkap:** Upload firmware utama; verifikasi slide dan pembacaan sensor.

### Masalah Umum & Perbaikan
| Masalah | Penyebab | Perbaikan |
|---------|----------|-----------|
| OLED Kosong | Kabel I2C longgar | Pasang ulang jumper, cek pull-up |
| DHT NaN | Pelanggaran timing | Tambah delay >2 detik antar baca |
| Error I2C | Konflik alamat | Scan I2C (alamat 0x3C untuk OLED) |
| Penurunan Daya | Kabel panjang | Pendekkan jumper, tambah kapasitor |

---

## 📝 Referensi
- [Panduan Adafruit SSD1306](https://learn.adafruit.com/monochrome-oled-breakouts/wiring-128x64-oleds)
- [DHT22 dengan ESP32](https://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-sensor-arduino-ide/)
- [Pin I2C ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html)

*Terakhir Diperbarui: 06 November 2025*
