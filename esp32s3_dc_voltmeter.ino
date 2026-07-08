/*******************************************************************************
 * ESP32-S3 Industrial DC Voltmeter
 * 4-channel (2 devices × IN/OUT) 0-5V bar-graph display, LVGL + Arduino_GFX.
 *
 * Display driver / pins reused from the Waveshare board's own
 * "02_gfx_helloworld" sample (ST7789 over SPI, landscape 320×240).
 *
 * Libraries required (Arduino Library Manager):
 *   - GFX Library for Arduino  (moononournation/Arduino_GFX)
 *   - lvgl                     (v8.3.x — NOT v9, the API differs)
 *
 * IMPORTANT: lv_conf.h must be placed in your Arduino libraries folder
 * (sibling to the "lvgl" library folder), NOT inside this sketch.
 ******************************************************************************/
#include "config.h"
#include "display_setup.h"
#include "ui.h"
#include "adc_task.h"
#include <lvgl.h>

/*--------------------------------------------------------------------------
 * LVGL tick handling
 *
 * If LV_TICK_CUSTOM is 1 in lv_conf.h, LVGL gets its tick automatically
 * from the custom callback (typically millis()), so no manual lv_tick_inc()
 * is needed.
 *
 * If LV_TICK_CUSTOM is 0, we must call lv_tick_inc() ourselves.
 *--------------------------------------------------------------------------*/
#if LV_TICK_CUSTOM == 0
static uint32_t lv_last_tick = 0;

static void lv_tick_update()
{
    uint32_t now = millis();
    uint32_t elapsed = now - lv_last_tick;
    if (elapsed > 0) {
        lv_tick_inc(elapsed);
        lv_last_tick = now;
    }
}
#endif

/*--------------------------------------------------------------------------
 * Arduino setup
 *--------------------------------------------------------------------------*/
void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println("ESP32-S3 DC Voltmeter starting...");

    display_and_lvgl_init();
    Serial.println("Display + LVGL init OK");

    ui_create();
    Serial.println("UI created");

    adc_task_start();
    Serial.println("ADC task started");

#if LV_TICK_CUSTOM == 0
    lv_last_tick = millis();
#endif

    Serial.println("Ready.");
}

/*--------------------------------------------------------------------------
 * Arduino loop — LVGL rendering + periodic UI value updates
 *--------------------------------------------------------------------------*/
void loop()
{
#if LV_TICK_CUSTOM == 0
    lv_tick_update();
#endif

    lv_timer_handler();

    // Update displayed values at the ADC sample rate
    static uint32_t last_ui_update = 0;
    uint32_t now = millis();
    if (now - last_ui_update >= (1000 / ADC_SAMPLE_HZ)) {
        last_ui_update = now;
        ui_update_values(g_v_a_in, g_v_a_out, g_v_b_in, g_v_b_out);
    }

    delay(1);
}
