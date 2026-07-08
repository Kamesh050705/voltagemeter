# ⚡ ESP32-S3 Industrial DC Voltmeter

A 4-channel (2 devices × IN/OUT) 0–5V DC voltmeter with a premium industrial dashboard UI, built on an ESP32-S3 with a Waveshare ST7789 SPI display (320×240).

![UI Preview](docs/ui_mockup.png)

---

## 📋 Features

- **4 simultaneous voltage channels** (0–5V each)
- **Device A** (IN + OUT) — emerald green bars
- **Device B** (IN + OUT) — amber bars
- Premium dark industrial dashboard UI via LVGL
- EMA-filtered ADC readings for stable display
- 50 Hz sampling rate with 6× oversampling
- FreeRTOS background ADC task (pinned to core 1)

---

## 🔧 Hardware Required

| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32-S3 Dev Board (Waveshare) | 1 | With built-in ST7789 240×320 SPI display |
| 12kΩ resistor | 4 | R1 (top) for each voltage divider |
| 18kΩ resistor | 4 | R2 (bottom) for each voltage divider |
| Breadboard / PCB | 1 | For the divider circuits |
| Jumper wires | ~20 | For connections |

---

## 🔌 Wiring Guide

For a highly detailed, step-by-step breadboarding tutorial and safety instructions, please see the [**Detailed Wiring Guide**](WIRING.md).

### Display (Built-in — No Wiring Needed)

The ST7789 SPI display is built into the Waveshare ESP32-S3 board. These pins are used internally:

| Signal | GPIO | Notes |
|--------|------|-------|
| SCLK | 39 | SPI Clock |
| MOSI | 38 | SPI Data Out |
| MISO | 40 | SPI Data In |
| DC | 42 | Data/Command select |
| CS | 45 | Chip Select |
| RST | -1 | Not connected (internal reset) |
| Backlight | 1 | Active HIGH |

> ⚠️ **Do NOT use GPIO 38, 39, 40, 42, 45, or 1** for anything else — they're reserved for the display.

---

### ADC Channels — Voltage Divider Wiring

Each 0–5V input signal must be scaled down to 0–3V using a resistor voltage divider before connecting to the ESP32-S3 ADC pins. The ESP32 ADC can only safely measure up to ~3.3V.

#### Voltage Divider Circuit (per channel)

```
    External Signal (0–5V)
           │
           │
          ┌┴┐
          │ │ R1 = 12kΩ
          │ │
          └┬┘
           │
           ├──────────── To ESP32 ADC Pin (0–3V)
           │
          ┌┴┐
          │ │ R2 = 18kΩ
          │ │
          └┬┘
           │
          GND
```

**How it works:**
```
Vadc = Vin × R2 / (R1 + R2)
Vadc = 5.0V × 18k / (12k + 18k)
Vadc = 5.0V × 0.6
Vadc = 3.0V  ✅ (safely within ADC range)
```

The firmware multiplies the ADC reading back by `(R1 + R2) / R2 = 1.667` to recover the original 0–5V value.

---

### Channel Pin Assignments

| Channel | Function | ESP32 GPIO | ADC Channel | Color on UI |
|---------|----------|------------|-------------|-------------|
| CH1 | Device A — Input | **GPIO 4** | ADC1_CH3 | 🟢 Emerald Green |
| CH2 | Device A — Output | **GPIO 5** | ADC1_CH4 | 🟢 Emerald Green |
| CH3 | Device B — Input | **GPIO 6** | ADC1_CH5 | 🟠 Amber |
| CH4 | Device B — Output | **GPIO 7** | ADC1_CH6 | 🟠 Amber |

> All 4 pins are on **ADC1**, so Wi-Fi (which uses ADC2) will not interfere with readings.

---

### Complete Wiring Diagram

```
                        ESP32-S3 (Waveshare)
                    ┌──────────────────────────┐
                    │                          │
   Device A IN ─────┤ GPIO 4  (ADC1_CH3)      │
   (via divider)    │                          │
                    │                          │
   Device A OUT ────┤ GPIO 5  (ADC1_CH4)      │
   (via divider)    │                          │
                    │                          │
   Device B IN ─────┤ GPIO 6  (ADC1_CH5)      │
   (via divider)    │                          │
                    │                          │
   Device B OUT ────┤ GPIO 7  (ADC1_CH6)      │
   (via divider)    │                          │
                    │                          │
           GND ─────┤ GND                     │
                    │                          │
                    │      ┌──────────┐        │
                    │      │ ST7789   │        │
                    │      │ 320×240  │        │
                    │      │ Display  │        │
                    │      │(built-in)│        │
                    │      └──────────┘        │
                    └──────────────────────────┘
```

### Wiring One Channel (Example: Device A IN → GPIO 4)

```
   Signal Source (0–5V)
        │
        │  Red wire
        │
       ┌┴┐
       │ │ 12kΩ (Brown-Red-Orange)
       └┬┘
        │
        ├───── Yellow wire ──── GPIO 4 (ESP32-S3)
        │
       ┌┴┐
       │ │ 18kΩ (Brown-Grey-Orange)
       └┬┘
        │
        │  Black wire
        │
       GND ──────────────────── GND (ESP32-S3)
```

> **Repeat this circuit 4 times** — one for each channel (GPIO 4, 5, 6, 7).

---

## ⚠️ Important Notes

1. **Always connect GND first** before applying any voltage
2. **Never apply more than 5V** to the divider input — higher voltages will damage the ESP32
3. **Use 1% tolerance resistors** for better accuracy (or calibrate in `config.h`)
4. The voltage divider must share a **common ground** with the ESP32-S3
5. Keep wires short to minimize noise on ADC readings

---

## 📦 Software Setup

### 1. Install Arduino IDE Libraries

| Library | Install via |
|---------|-------------|
| **GFX Library for Arduino** (by moononournation) | Library Manager → search `Arduino_GFX` |
| **lvgl** (v8.3.x) | Library Manager → search `lvgl` |

### 2. Configure `lv_conf.h`

Copy `lvgl/lv_conf_template.h` → `Arduino/libraries/lv_conf.h` (sibling of `lvgl/` folder), then:

```c
#if 1  // Change 0 → 1 to enable

#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        0
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
```

### 3. Board Settings (Tools menu)

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240MHz |
| Flash Size | Match your board (4/8/16MB) |
| Partition Scheme | Huge APP (3MB No OTA) |
| PSRAM | OPI PSRAM (if available) |

### 4. Upload

1. Open `esp32s3_dc_voltmeter.ino`
2. Verify/Compile (Ctrl+R)
3. Upload (Ctrl+U)
4. Serial Monitor at 115200 baud should show:
   ```
   ESP32-S3 DC Voltmeter starting...
   Display + LVGL init OK
   UI created
   ADC task started
   Ready.
   ```

---

## 🔧 Calibration

To fine-tune accuracy per channel, edit the calibration multipliers in [`config.h`](config.h):

```c
#define CAL_A_IN   1.0f   // Device A Input calibration
#define CAL_A_OUT  1.0f   // Device A Output calibration
#define CAL_B_IN   1.0f   // Device B Input calibration
#define CAL_B_OUT  1.0f   // Device B Output calibration
```

**Steps:**
1. Apply a known reference voltage (e.g., 3.000V from a bench supply)
2. Read the displayed value
3. Calculate: `CAL = actual_voltage / displayed_voltage`
4. Update the corresponding `CAL_*` value in `config.h`

---

## 📁 File Structure

```
esp32s3_dc_voltmeter/
├── esp32s3_dc_voltmeter.ino   # Main sketch (setup + loop)
├── config.h                    # Pin mappings, ADC config, calibration
├── display_setup.h / .cpp      # ST7789 + LVGL display driver init
├── adc_task.h / .cpp           # FreeRTOS ADC sampling + EMA filter
├── ui.h / .cpp                 # Premium industrial dashboard UI
├── lv_conf.h                   # LVGL configuration
├── WIRING.md                   # Detailed wiring guide
├── FLOWCHART.md                # System architecture flowchart
└── README.md                   # This file
```

---

## 📄 License

MIT License — free to use, modify, and distribute.
