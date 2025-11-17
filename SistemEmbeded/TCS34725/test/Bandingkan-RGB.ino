#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <math.h>  // Untuk sqrt() di jarak Euclidean

// Inisialisasi sensor
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// Rata-rata dari data sampel (centroids)
struct ColorCategory {
  float r, g, b;
  const char* name;
};

ColorCategory categories[3] = {
  {175.8, 54.3, 61.2, "RED"},
  {105.8, 99.4, 60.9, "GREEN"},
  {107.0, 88.5, 77.0, "BLUE"}
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!tcs.begin()) {
    Serial.println("ERROR: Sensor tidak ditemukan!");
    while (1) delay(1000);
  }
  Serial.println("OK - Deteksi Warna Aktif");
}

void loop() {
  uint16_t r_raw, g_raw, b_raw, c_raw;
  tcs.getRawData(&r_raw, &g_raw, &b_raw, &c_raw);

  // Normalisasi (raw langsung, tanpa buffer untuk simplicity)
  float avg_r = r_raw;
  float avg_g = g_raw;
  float avg_b = b_raw;  // FIX: Tambah deklarasi ini!
  float avg_c = c_raw;
  float r_norm = 0, g_norm = 0, b_norm = 0;
  if (avg_c > 10) {
    r_norm = (avg_r / avg_c) * 255.0;
    g_norm = (avg_g / avg_c) * 255.0;
    b_norm = (avg_b / avg_c) * 255.0;  // Sekarang avg_b sudah ada
  }
  r_norm = constrain(r_norm, 0, 255);
  g_norm = constrain(g_norm, 0, 255);
  b_norm = constrain(b_norm, 0, 255);

  // Hitung jarak Euclidean ke setiap kategori
  float min_dist = 999999;
  const char* detected_color = "UNKNOWN";
  for (int i = 0; i < 3; i++) {
    float dist = sqrt(
      pow(r_norm - categories[i].r, 2) +
      pow(g_norm - categories[i].g, 2) +
      pow(b_norm - categories[i].b, 2)
    );
    if (dist < min_dist) {
      min_dist = dist;
      detected_color = categories[i].name;
    }
  }

  // Output: R,G,B -> WARNA (jarak)
  Serial.print((int)r_norm);
  Serial.print(",");
  Serial.print((int)g_norm);
  Serial.print(",");
  Serial.print((int)b_norm);
  Serial.print(" -> ");
  Serial.print(detected_color);
  Serial.print(" (");
  Serial.print(min_dist, 1);
  Serial.println(")");

  delay(200);  // Delay untuk tes stabil
}
