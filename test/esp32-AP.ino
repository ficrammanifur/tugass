#include <WiFi.h>
#include <WiFiManager.h>

WiFiManager wm;

void setup() {
  Serial.begin(115200);
  delay(500);

  // Mode gabungan: Station (konek ke router) + Access Point (untuk konfigurasi)
  WiFi.mode(WIFI_AP_STA);

  // 1️⃣ Buat Access Point manual
  const char* ap_ssid = "Wifi-setup-winda";
  const char* ap_pass = "12345678";

  Serial.println("Menyalakan Access Point...");
  bool apStarted = WiFi.softAP(ap_ssid, ap_pass);
  if (apStarted) {
    Serial.print("✅ Access Point aktif: ");
    Serial.println(ap_ssid);
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("❌ Gagal menyalakan Access Point!");
  }

  // 2️⃣ Jalankan WiFiManager portal agar bisa setup jaringan router
  wm.setConfigPortalBlocking(false); // supaya tidak ngebekuin program
  wm.autoConnect(ap_ssid, ap_pass);

  // 3️⃣ Coba konek ke jaringan WiFi utama (jika disimpan)
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🌐 Terkoneksi ke WiFi utama!");
    Serial.print("IP Lokal: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("⚠️ Tidak terkoneksi WiFi utama, tetap dalam mode offline + portal aktif.");
  }
}

void loop() {
  // Pastikan WiFiManager portal tetap aktif selama ESP berjalan
  wm.process();

  // Jika koneksi hilang, tetap aktifkan AP agar user bisa konfigurasi ulang
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.softAP("Wifi-setup-winda", "12345678");
  }

  delay(1000); // jeda 1 detik
}
