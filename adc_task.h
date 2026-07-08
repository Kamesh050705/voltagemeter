#pragma once

// Latest calibrated readings, in volts (0-5V range), updated by the ADC task.
extern volatile float g_v_a_in;
extern volatile float g_v_a_out;
extern volatile float g_v_b_in;
extern volatile float g_v_b_out;

// Starts the FreeRTOS task that samples all 4 channels continuously.
void adc_task_start();
