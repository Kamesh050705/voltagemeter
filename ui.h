#pragma once

// Build the premium industrial dashboard UI using LVGL.
void ui_create();

// Push new readings (volts, 0-5 range) into the 4 bar rows.
// Order: Device A IN, Device A OUT, Device B IN, Device B OUT.
void ui_update_values(float a_in, float a_out, float b_in, float b_out);
