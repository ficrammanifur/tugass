/*
  ESP32 — LEDs + Buzzer + 2 Push Buttons + Touch + MAX7219
*/

#include <Arduino.h>
#include <SPI.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

// ====== PIN SETUP ======
#define LED_RED_PIN     13    // LED merah
#define LED_GREEN_PIN   26    // LED hijau
#define BUZZER_PIN      27
#define BUTTON_RED_PIN  12    // push button merah
#define BUTTON_GREEN_PIN 14   // push button hijau
#define TOUCH_PIN       4     // SIG TTP223 ke GPIO4

// MAX7219 (4 modul FC16)
#define MAX_DEVICES   4
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_CS_PIN    5
#define MAX_CLK_PIN   18
#define MAX_DATA_PIN  23

// ====== OBJECT MATRIX ======
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, MAX_CS_PIN, MAX_DEVICES);

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  delay(500); // Allow time to open serial monitor

  Serial.println("Starting hardware test...");

  // Initial pin states
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(BUTTON_RED_PIN, INPUT_PULLUP);   // Pull-up: pressed = LOW
  pinMode(BUTTON_GREEN_PIN, INPUT_PULLUP); // Pull-up: pressed = LOW
  pinMode(TOUCH_PIN, INPUT);               // For TTP223 touch sensor

  // Hardware test: Blink LEDs and beep buzzer
  Serial.println("Testing LEDs and Buzzer...");
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_RED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_RED_PIN, LOW);
    delay(200);
  }
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_GREEN_PIN, HIGH);
    delay(200);
    digitalWrite(LED_GREEN_PIN, LOW);
    delay(200);
  }
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
  Serial.println("Hardware test complete. Check if LEDs blinked and buzzer beeped.");

  // MAX7219 setup
  matrix.begin();
  matrix.setIntensity(5);
  matrix.displayClear();
  matrix.displayText("HELLO WORLD!!", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

  Serial.println("System Ready! Wiring: Buttons to GND. Pressed = LOW.");
}

// ====== LOOP ======
void loop() {
  // --- BUTTON RED (direct control with pull-up) ---
  int redState = digitalRead(BUTTON_RED_PIN);
  digitalWrite(LED_RED_PIN, (redState == LOW) ? HIGH : LOW);  // LOW (pressed) -> LED ON

  // --- BUTTON GREEN (direct control with pull-up) ---
  int greenState = digitalRead(BUTTON_GREEN_PIN);
  digitalWrite(LED_GREEN_PIN, (greenState == LOW) ? HIGH : LOW);  // LOW (pressed) -> LED ON

  // --- TOUCH SENSOR (TTP223) ---
  int touchState = digitalRead(TOUCH_PIN);
  if (touchState == HIGH) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // --- DEBUG PRINTS (every 2 seconds) ---
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    Serial.print("Button Red raw: ");
    Serial.print(redState);
    Serial.print(" (LOW=pressed), Button Green raw: ");
    Serial.print(greenState);
    Serial.print(", Touch State: ");
    Serial.print(touchState);
    Serial.print(" (HIGH=touched), Buzzer: ");
    Serial.println(touchState ? "ON" : "OFF");
    lastPrint = millis();
  }

  // --- MAX7219 update ---
  if (matrix.displayAnimate()) matrix.displayReset();

  delay(10);
}
