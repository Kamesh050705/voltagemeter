#pragma once
#include <Arduino.h>

/*******************************************************************************
 * DISPLAY -- pins/driver taken directly from the board's own
 * 02_gfx_helloworld.ino sample (ST7789, SPI, IPS panel).
 ******************************************************************************/
#define LCD_PIN_SCLK 39
#define LCD_PIN_MOSI 38
#define LCD_PIN_MISO 40
#define LCD_PIN_DC   42
#define LCD_PIN_RST  -1
#define LCD_PIN_CS   45
#define LCD_PIN_BL   1

#define LCD_ROTATION      1     // landscape, matches sample
#define LCD_H_RES_NATIVE  240
#define LCD_V_RES_NATIVE  320

#define SCREEN_W 320
#define SCREEN_H 240

/*******************************************************************************
 * ADC CHANNELS
 * These 4 GPIOs are free on the ESP32-S3 module (not used by the SPI panel
 * bus above or the backlight pin). All are ADC1 pins, so Wi-Fi (which uses
 * ADC2 internally) will not interfere with readings.
 *   GPIO4  = ADC1_CH3   -> Device A, IN
 *   GPIO5  = ADC1_CH4   -> Device A, OUT
 *   GPIO6  = ADC1_CH5   -> Device B, IN
 *   GPIO7  = ADC1_CH6   -> Device B, OUT
 * If your specific carrier board already uses these for something else,
 * change them here -- nothing else in the firmware needs to change.
 ******************************************************************************/
#define ADC_PIN_A_IN   4
#define ADC_PIN_A_OUT  5
#define ADC_PIN_B_IN   6
#define ADC_PIN_B_OUT  7

#define ADC_SAMPLE_HZ       50   // sampling / UI refresh rate (spec asks for 30-60Hz)
#define ADC_OVERSAMPLE      6    // samples averaged per reading (keeps latency low)
#define ADC_RESOLUTION_BITS 12
#define ADC_VREF_APPROX     3.30f  // approx ESP32-S3 ADC full-scale at 11dB attenuation

/*******************************************************************************
 * ADC ATTENUATION COMPATIBILITY
 * ESP32 Arduino core 2.x uses ADC_11db, while 3.x uses ADC_ATTEN_DB_11.
 * This macro ensures compilation on both versions.
 ******************************************************************************/
#if !defined(ADC_ATTEN_DB_11) && defined(ADC_11db)
  #define ADC_ATTEN_DB_11  ADC_11db
#elif !defined(ADC_ATTEN_DB_11) && !defined(ADC_11db)
  #define ADC_ATTEN_DB_11  3   // raw enum value as last-resort fallback
#endif

/*******************************************************************************
 * EMA (Exponential Moving Average) SMOOTHING
 * Alpha = 0.0..1.0.  Higher = more responsive, lower = smoother.
 * 0.25 gives a nice balance at 50 Hz sampling.
 ******************************************************************************/
#define ADC_EMA_ALPHA  0.25f

/*******************************************************************************
 * VOLTAGE DIVIDER SCALING
 * Each 0-5V input is scaled down by an external resistor divider (see README
 * for the recommended circuit) before it reaches the ADC pin:
 *   Vadc = Vin * R2 / (R1 + R2)
 * With R1(top)=12k, R2(bottom)=18k:  ratio = 18/30 = 0.6
 *   Vin = 5.0V  ->  Vadc = 3.0V   (safely inside the ADC's usable/linear range)
 *   Vin = 0.0V  ->  Vadc = 0.0V
 * Firmware multiplies the measured ADC voltage back up by (R1+R2)/R2 to
 * recover the original 0-5V signal.
 ******************************************************************************/
#define DIVIDER_R1_OHMS 12000.0f
#define DIVIDER_R2_OHMS 18000.0f
#define DIVIDER_SCALE   ((DIVIDER_R1_OHMS + DIVIDER_R2_OHMS) / DIVIDER_R2_OHMS) // 1.6667

// Per-channel fine-calibration multipliers. Leave at 1.0f and tune individually
// against a known reference voltage to null out real resistor tolerance.
#define CAL_A_IN   1.0f
#define CAL_A_OUT  1.0f
#define CAL_B_IN   1.0f
#define CAL_B_OUT  1.0f
