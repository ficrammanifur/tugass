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
