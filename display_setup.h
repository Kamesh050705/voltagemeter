#pragma once
#include <Arduino_GFX_Library.h>

// Shared GFX instance -- used by the LVGL flush callback to push pixels
// to the ST7789 display over SPI.
extern Arduino_GFX *gfx;

// Initialises the display hardware (matching the Waveshare helloworld sample),
// then sets up LVGL with double-buffered partial rendering.
void display_and_lvgl_init();
