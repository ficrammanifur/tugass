#include <Wire.h>
#include <Adafruit_TCS34725.h>

// Inisialisasi sensor (integration time & gain disesuaikan)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

// Buffer untuk moving average
const int SAMPLES = 10;
uint32_t r_buffer[SAMPLES] = {0};
uint32_t g_buffer[SAMPLES] = {0};
uint32_t b_buffer[SAMPLES] = {0};
uint32_t c_buffer[SAMPLES] = {0};

int buf_index = 0;
bool buffer_full = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!tcs.begin()) {
    Serial.println("ERROR: Sensor tidak ditemukan!");  // Tetap ada error message di setup
    while (1) delay(1000);
  }

  Serial.println("OK");  // Hanya ini di setup, lalu langsung ke loop
}

void loop() {
  uint16_t r, g, b, c;

  tcs.getRawData(&r, &g, &b, &c);

  // Masukkan ke buffer (circular)
  r_buffer[buf_index] = r;
  g_buffer[buf_index] = g;
  b_buffer[buf_index] = b;
  c_buffer[buf_index] = c;

  buf_index++;
  if (buf_index >= SAMPLES) {
    buf_index = 0;
    buffer_full = true;
  }

  // Hitung rata-rata
  uint32_t sum_r = 0, sum_g = 0, sum_b = 0, sum_c = 0;
  int count = buffer_full ? SAMPLES : buf_index;

  for (int i = 0; i < count; i++) {
    sum_r += r_buffer[i];
    sum_g += g_buffer[i];
    sum_b += b_buffer[i];
    sum_c += c_buffer[i];
  }

  float avg_r = (float)sum_r / count;
  float avg_g = (float)sum_g / count;
  float avg_b = (float)sum_b / count;
  float avg_c = (float)sum_c / count;

  // Normalisasi (hindari pembagian 0)
  float r_norm = 0, g_norm = 0, b_norm = 0;
  if (avg_c > 10) {
    r_norm = (avg_r / avg_c) * 255.0;
    g_norm = (avg_g / avg_c) * 255.0;
    b_norm = (avg_b / avg_c) * 255.0;
  }

  // Batasi 0-255
  r_norm = constrain(r_norm, 0, 255);
  g_norm = constrain(g_norm, 0, 255);
  b_norm = constrain(b_norm, 0, 255);

  // Output langsung: R,G,B
  Serial.print((int)r_norm);
  Serial.print(",");
  Serial.print((int)g_norm);
  Serial.print(",");
  Serial.println((int)b_norm);

  delay(100);
}
