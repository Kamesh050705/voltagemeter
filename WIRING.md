# 🔌 Detailed Wiring Guide: ESP32-S3 DC Voltmeter

This guide explains exactly how to wire the external voltage sources (up to 5V) to your ESP32-S3 board safely and accurately.

---

## 1. The Core Problem: Why We Need Resistors
The ESP32-S3's Analog-to-Digital Converter (ADC) pins can only safely measure voltages between **0V and ~3.3V** (depending on attenuation settings). 
If you connect a 5V signal directly to an ESP32 GPIO pin, **you will permanently damage the microcontroller.**

To solve this, we use a **Voltage Divider** circuit for each channel. This uses two resistors to proportionally "shrink" the 0–5V signal down to a safe 0–3V range that the ESP32 can read. The software then mathematically scales it back up for the display.

---

## 2. The Voltage Divider Circuit

You will need to build this simple circuit **four times** (once for each channel).

### Required Components (Per Channel)
*   **R1 (Top Resistor):** 12 kΩ (12,000 Ohms)
*   **R2 (Bottom Resistor):** 18 kΩ (18,000 Ohms)
*   Ideally, use 1% tolerance metal film resistors for the best accuracy.

### Schematic Diagram

```text
  [External Signal to Measure: 0-5V MAX]
                    │
                    │
                   ┌┴┐
                   │ │ R1 (12kΩ)
                   │ │
                   └┬┘
                    │
                    ├──────────────────────────► [To ESP32-S3 GPIO Pin]
                    │                            (Voltage here is 0-3V)
                   ┌┴┐
                   │ │ R2 (18kΩ)
                   │ │
                   └┬┘
                    │
                    │
                  [GND] ◄──────────────────────► [ESP32-S3 GND Pin]
             (Common Ground)                     (Must be connected!)
```

### How the Math Works:
*   Max Input = 5.0V
*   Voltage at ESP32 Pin = Input * (R2 / (R1 + R2))
*   Voltage at ESP32 Pin = 5.0V * (18k / (12k + 18k))
*   Voltage at ESP32 Pin = 5.0V * (18/30) = **3.0V** (Safe!)

---

## 3. ESP32-S3 Pin Assignments

Here is exactly where each of the 4 scaled signals should connect on your ESP32-S3 board. We specifically chose these pins because they belong to `ADC1`, meaning they won't interfere with Wi-Fi if you decide to add it later (Wi-Fi uses `ADC2`).

| Measurement Channel | ESP32-S3 Pin | Notes |
| :--- | :--- | :--- |
| **Device A - IN** | `GPIO 4` | Connect center of divider 1 here |
| **Device A - OUT** | `GPIO 5` | Connect center of divider 2 here |
| **Device B - IN** | `GPIO 6` | Connect center of divider 3 here |
| **Device B - OUT** | `GPIO 7` | Connect center of divider 4 here |
| **Ground (Common)** | `GND` | Connect all divider grounds here |

---

## 4. Step-by-Step Breadboard Example (Wiring Device A - IN)

Let's wire up the first channel (Device A - IN) as a practical example.

1.  **Common Ground (Crucial Step):**
    *   Connect a wire from any `GND` pin on the ESP32-S3 to the blue/negative rail on your breadboard.
    *   Connect the ground (negative) terminal of your 0-5V voltage source to this same blue/negative rail.
2.  **Place the Resistors:**
    *   Take the **12kΩ** resistor (R1). Plug one leg into a new row on the breadboard (let's call it Row 10). Plug the other leg into a row below it (Row 15).
    *   Take the **18kΩ** resistor (R2). Plug one leg into the *same row* as R1's bottom leg (Row 15). Plug the other leg into the ground rail.
3.  **Connect the Signal (Input):**
    *   Connect the positive wire from your 0-5V source to the top of R1 (Row 10).
4.  **Connect to ESP32 (Output):**
    *   Connect a wire from the junction between the two resistors (Row 15) to **GPIO 4** on your ESP32-S3.

**Repeat steps 2-4** for the other three channels, connecting their junctions to GPIO 5, 6, and 7 respectively, and tying all their R2 bottom legs to the common ground rail.

---

## 5. Important Safety & Accuracy Tips

*   ⚠️ **Never exceed 5V on the input.** If your source spikes to 6V, the voltage at the ESP32 pin will hit 3.6V, which can damage the ADC or the whole chip. If you need to measure higher voltages (e.g., 12V or 24V batteries), you **must recalculate and change the resistor values**.
*   **Calibration:** Resistors are rarely perfect. A "12k" resistor might actually be 11.8k or 12.2k. This will make your readings slightly off. To fix this, use a multimeter to measure a known voltage (like a 1.5V battery or a 3.3V pin), see what the screen displays, and adjust the `CAL_A_IN`, `CAL_A_OUT`, etc. values in `config.h`. 
    *   *Example:* Real voltage is 3.30V. Screen says 3.20V. Change `CAL_A_IN` from `1.0f` to `1.03125f` (which is 3.30 / 3.20).
*   **Keep Wires Short:** Long wires act like antennas and pick up electrical noise, causing the numbers on the screen to jump around.
*   **The display wiring** is entirely handled internally by the Waveshare board. Do not connect anything to GPIOs 38, 39, 40, 42, 45, or 1.
