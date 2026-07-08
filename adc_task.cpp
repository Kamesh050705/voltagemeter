/*******************************************************************************
 * adc_task.cpp
 *
 * FreeRTOS task that continuously samples 4 ADC channels (2 devices × IN/OUT)
 * and applies voltage-divider scaling + per-channel calibration.
 *
 * Includes an exponential moving average (EMA) filter for smooth UI display.
 ******************************************************************************/
#include "adc_task.h"
#include "config.h"
#include <Arduino.h>

volatile float g_v_a_in  = 0.0f;
volatile float g_v_a_out = 0.0f;
volatile float g_v_b_in  = 0.0f;
volatile float g_v_b_out = 0.0f;

static const int ADC_MAX_VAL = (1 << ADC_RESOLUTION_BITS) - 1;

/*--------------------------------------------------------------------------
 * Read one ADC channel with oversampling, divider scaling, and calibration.
 *--------------------------------------------------------------------------*/
static float read_channel_raw(int pin, float cal)
{
    uint32_t sum = 0;
    for (int i = 0; i < ADC_OVERSAMPLE; i++) {
        sum += analogRead(pin);
    }
    float raw  = (float)sum / ADC_OVERSAMPLE;
    float vadc = (raw / ADC_MAX_VAL) * ADC_VREF_APPROX;
    return vadc * DIVIDER_SCALE * cal;
}

/*--------------------------------------------------------------------------
 * EMA filter:  filtered = alpha * new_sample + (1-alpha) * filtered
 *--------------------------------------------------------------------------*/
static float ema_filter(float prev, float sample)
{
    return ADC_EMA_ALPHA * sample + (1.0f - ADC_EMA_ALPHA) * prev;
}

/*--------------------------------------------------------------------------
 * ADC sampling task  (runs on core 1, priority 2)
 *--------------------------------------------------------------------------*/
static void adc_task(void *arg)
{
    const int pins[4] = {ADC_PIN_A_IN, ADC_PIN_A_OUT,
                         ADC_PIN_B_IN, ADC_PIN_B_OUT};

    for (int i = 0; i < 4; i++) {
        pinMode(pins[i], INPUT);
        analogSetPinAttenuation(pins[i], (adc_attenuation_t)ADC_ATTEN_DB_11);
    }
    analogReadResolution(ADC_RESOLUTION_BITS);

    // Seed the EMA with a first real reading
    float filtered[4];
    const float cal[4] = {CAL_A_IN, CAL_A_OUT, CAL_B_IN, CAL_B_OUT};
    for (int i = 0; i < 4; i++) {
        filtered[i] = read_channel_raw(pins[i], cal[i]);
    }

    const TickType_t period = pdMS_TO_TICKS(1000 / ADC_SAMPLE_HZ);

    for (;;) {
        for (int i = 0; i < 4; i++) {
            float sample = read_channel_raw(pins[i], cal[i]);
            filtered[i]  = ema_filter(filtered[i], sample);
        }

        g_v_a_in  = filtered[0];
        g_v_a_out = filtered[1];
        g_v_b_in  = filtered[2];
        g_v_b_out = filtered[3];

        vTaskDelay(period);
    }
}

void adc_task_start()
{
    // Pinned to core 1 (keeps Wi-Fi/BT housekeeping on core 0 from
    // ever delaying a sample).
    xTaskCreatePinnedToCore(adc_task, "adc_task", 4096, NULL, 2, NULL, 1);
}
