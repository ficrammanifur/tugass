#ifndef EYE_ANIMATION_H
#define EYE_ANIMATION_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==== BITMAP DEFINITIONS (CONTOH RINGKAS - TAMBAHKAN SEMUA 90 FRAME KAMU) ====
// Frame 00 (contoh dari kode kamu)
const unsigned char epd_bitmap_00 [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // ... (lanjutkan 1024 bytes total per frame dari data asli kamu)
  // Akhiri dengan baris terakhir seperti di kode kamu
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Frame 01 (CONTOH - GANTI DENGAN DATA ASLI DARI IMAGE2CPP)
const unsigned char epd_bitmap_01 [] PROGMEM = {
  // Copy hex data frame 1 kamu di sini (sama format, 1024 bytes)
  0x00, 0x00, 0x00, 0x00, /* ... isi lengkap ... */ 0x00, 0x00
};

// Frame 02 (CONTOH - GANTI DENGAN DATA ASLI)
const unsigned char epd_bitmap_02 [] PROGMEM = {
  // Copy hex data frame 2 kamu di sini
  0x00, 0x00, 0x00, 0x00, /* ... isi lengkap ... */ 0x00, 0x00
};

// ... (Tambahkan epd_bitmap_03 sampai epd_bitmap_89 dengan pola yang sama)

// Array of all bitmaps (sesuaikan LEN kalau kurangin frame buat test)
const int epd_bitmap_allArray_LEN = 3;  // CONTOH: 3 frame dulu, ganti ke 90 nanti
const unsigned char* epd_bitmap_allArray[90] = {  // Tetap 90 slot, tapi isi cuma 3 dulu
  epd_bitmap_00, epd_bitmap_01, epd_bitmap_02,
  // ... (isi epd_bitmap_03 sampai _89 di slot selanjutnya)
  // Untuk slot kosong sementara, bisa pakai epd_bitmap_00 sebagai fallback
};

class EyeAnimation {
private:
  int frame;                          // Frame saat ini (0 - 89)
  unsigned long lastFrameChange;      // Waktu ganti frame terakhir
  static const unsigned long FRAME_INTERVAL = 100;  // Ganti frame tiap 100ms (10 FPS - sesuaikan)

public:
  EyeAnimation() : frame(0), lastFrameChange(0) {}  // Init di constructor
  
  void begin() {
    // Nggak perlu random, langsung cycle kayak loop() U8g2-mu
  }
  
  // Update: Cycle frame non-blocking (mirip loop() di kode kamu)
  void update() {
    unsigned long now = millis();
    if (now - lastFrameChange >= FRAME_INTERVAL) {
      lastFrameChange = now;
      frame++;  // Increase frame
      if (frame >= epd_bitmap_allArray_LEN) {  // Reset kalau udah habis
        frame = 0;
      }
    }
  }
  
  // Draw: Render bitmap ke display (mirip u8g2.drawXBMP)
  void draw(Adafruit_SSD1306 &display) {
    // Nggak perlu clearDisplay() di sini - biar drawEyeScreen() handle
    // Render full-screen bitmap (128x64)
    display.drawXBMP(0, 0, 128, 64, epd_bitmap_allArray[frame]);
  }
};

#endif
