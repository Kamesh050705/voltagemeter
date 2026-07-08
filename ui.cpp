/*******************************************************************************
 * ui.cpp — Premium Industrial DC Voltmeter Dashboard
 *
 * Fully LVGL-rendered (no direct Arduino_GFX framebuffer writes).
 * All UI elements are LVGL objects, so no pixel-overwrite conflicts.
 *
 * Layout (320×240 landscape):
 *
 *  ┌──────────────────────────────────────────────────────┐
 *  │  ⚡ DC VOLTMETER                      ESP32-S3      │  <- Header (24px)
 *  │──────────────────────────────────────────────────────│
 *  │  ┌─ DEVICE A ─────────────────────────────────────┐ │
 *  │  │  IN  ████████████████░░░░░░░  3.27V            │ │  <- Row (44px)
 *  │  │  OUT ██████████░░░░░░░░░░░░░  2.14V            │ │  <- Row (44px)
 *  │  └────────────────────────────────────────────────┘ │
 *  │  ┌─ DEVICE B ─────────────────────────────────────┐ │
 *  │  │  IN  ████████████████████░░░  4.51V            │ │  <- Row (44px)
 *  │  │  OUT ████░░░░░░░░░░░░░░░░░░░  0.85V            │ │  <- Row (44px)
 *  │  └────────────────────────────────────────────────┘ │
 *  │  0V     1V     2V     3V     4V     5V              │  <- Scale (18px)
 *  └──────────────────────────────────────────────────────┘
 *
 ******************************************************************************/
#include "ui.h"
#include "config.h"
#include <lvgl.h>
#include <math.h>

/*==========================================================================
 * Color Palette — dark industrial theme
 *==========================================================================*/
#define COL_BG_DEEP      lv_color_hex(0x0A0E14)   // deep navy-black background
#define COL_BG_PANEL      lv_color_hex(0x111820)   // panel interior
#define COL_BORDER        lv_color_hex(0x1E2A38)   // subtle panel border
#define COL_BORDER_ACCENT lv_color_hex(0x2A3A4E)   // brighter border
#define COL_HEADER_BG     lv_color_hex(0x0D1117)   // header strip
#define COL_HEADER_LINE   lv_color_hex(0x1A8CFF)   // blue accent line under header
#define COL_TRACK         lv_color_hex(0x0D1218)   // bar track (dark)
#define COL_TRACK_BORDER  lv_color_hex(0x1A2430)   // bar track border
#define COL_TICK_MAJOR    lv_color_hex(0x3A4A5A)   // major tick marks
#define COL_TICK_MINOR    lv_color_hex(0x1E2830)   // minor tick marks
#define COL_TEXT_DIM      lv_color_hex(0x5A6A7A)   // dim labels
#define COL_TEXT_MED      lv_color_hex(0x8899AA)   // medium labels
#define COL_TEXT_BRIGHT   lv_color_hex(0xE0E8F0)   // bright readout text
#define COL_SCALE_NUM     lv_color_hex(0x4A5A6A)   // scale numbers
#define COL_BADGE_BG      lv_color_hex(0x141C26)   // badge background
#define COL_BADGE_TEXT    lv_color_hex(0x4A6080)   // badge text

// Device accent colors
#define COL_DEV_A         lv_color_hex(0x00D68F)   // Device A: emerald green
#define COL_DEV_A_DIM     lv_color_hex(0x004D33)   // Device A: dim accent
#define COL_DEV_A_GLOW    lv_color_hex(0x00FF9F)   // Device A: glow
#define COL_DEV_B         lv_color_hex(0xFF8C42)   // Device B: warm amber
#define COL_DEV_B_DIM     lv_color_hex(0x4D2A13)   // Device B: dim accent
#define COL_DEV_B_GLOW    lv_color_hex(0xFFAA66)   // Device B: glow

/*==========================================================================
 * Layout Constants
 *==========================================================================*/
#define HEADER_H      24
#define PANEL_PAD_X   4
#define PANEL_GAP     3    // gap between device A and B panels
#define PANEL_W       (SCREEN_W - 2 * PANEL_PAD_X)
#define PANEL_INNER_H 88   // each device panel height

#define PANEL_A_Y     (HEADER_H + 2)
#define PANEL_B_Y     (PANEL_A_Y + PANEL_INNER_H + PANEL_GAP)

#define SCALE_Y       (PANEL_B_Y + PANEL_INNER_H + 2)
#define SCALE_H       18

// Inside each panel
#define ROW_H         39   // each row (IN or OUT) height within panel
#define ROW_0_Y       8    // first row y-offset inside panel (after header area)
#define ROW_1_Y       (ROW_0_Y + ROW_H + 2) // second row y-offset

// Bar geometry (inside each row)
#define BAR_LABEL_W   30   // "IN" / "OUT" label width
#define BAR_VALUE_W   52   // voltage readout width at right
#define BAR_X_OFFS    (BAR_LABEL_W + 2)
#define BAR_W         (PANEL_W - BAR_X_OFFS - BAR_VALUE_W - 12)
#define BAR_H         16
#define BAR_Y_OFFS    12   // bar y-offset within its row

/*==========================================================================
 * LVGL Object Handles
 *==========================================================================*/
static lv_obj_t *bar_obj[4];          // custom-drawn bar objects
static lv_obj_t *value_label[4];      // voltage readout labels
static float     bar_value[4];        // current bar fill (0..5V)
static float     last_shown[4] = {-1, -1, -1, -1};

// Device accent colors per row (A-IN, A-OUT, B-IN, B-OUT)
static lv_color_t row_color[4];
static lv_color_t row_color_dim[4];

/*==========================================================================
 * Styles (allocated once)
 *==========================================================================*/
static lv_style_t sty_panel;
static lv_style_t sty_row_label;
static lv_style_t sty_value;
static lv_style_t sty_device_header;
static lv_style_t sty_scale;
static bool styles_inited = false;

static void init_styles()
{
    if (styles_inited) return;
    styles_inited = true;

    // Panel container
    lv_style_init(&sty_panel);
    lv_style_set_bg_color(&sty_panel, COL_BG_PANEL);
    lv_style_set_bg_opa(&sty_panel, LV_OPA_COVER);
    lv_style_set_border_color(&sty_panel, COL_BORDER);
    lv_style_set_border_width(&sty_panel, 1);
    lv_style_set_border_opa(&sty_panel, LV_OPA_COVER);
    lv_style_set_radius(&sty_panel, 6);
    lv_style_set_pad_all(&sty_panel, 0);

    // "IN" / "OUT" labels
    lv_style_init(&sty_row_label);
    lv_style_set_text_color(&sty_row_label, COL_TEXT_DIM);
    lv_style_set_text_font(&sty_row_label, &lv_font_montserrat_12);

    // Voltage value readout
    lv_style_init(&sty_value);
    lv_style_set_text_color(&sty_value, COL_TEXT_BRIGHT);
    lv_style_set_text_font(&sty_value, &lv_font_montserrat_14);

    // Device header ("DEVICE A", "DEVICE B")
    lv_style_init(&sty_device_header);
    lv_style_set_text_font(&sty_device_header, &lv_font_montserrat_12);

    // Scale numbers
    lv_style_init(&sty_scale);
    lv_style_set_text_color(&sty_scale, COL_SCALE_NUM);
    lv_style_set_text_font(&sty_scale, &lv_font_montserrat_12);
}

/*==========================================================================
 * Custom Bar Draw Callback
 *
 * Draws: dark track → minor/major tick marks → colored fill bar.
 * Called by LVGL whenever the bar object's area needs repainting.
 *==========================================================================*/
static void bar_draw_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;

    lv_obj_t *obj = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(obj);

    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
    lv_area_t area;
    lv_obj_get_coords(obj, &area);

    lv_coord_t w = area.x2 - area.x1;
    lv_coord_t h = area.y2 - area.y1;

    /* --- Track background with rounded corners --- */
    lv_draw_rect_dsc_t track_dsc;
    lv_draw_rect_dsc_init(&track_dsc);
    track_dsc.bg_color    = COL_TRACK;
    track_dsc.bg_opa      = LV_OPA_COVER;
    track_dsc.border_color = COL_TRACK_BORDER;
    track_dsc.border_width = 1;
    track_dsc.border_opa  = LV_OPA_COVER;
    track_dsc.radius      = 3;
    lv_draw_rect(draw_ctx, &track_dsc, &area);

    /* --- Tick marks: major every 1V, minor every 0.5V --- */
    lv_draw_line_dsc_t tick_dsc;
    lv_draw_line_dsc_init(&tick_dsc);
    tick_dsc.width = 1;

    for (int v10 = 5; v10 <= 50; v10 += 5) {
        bool major = (v10 % 10 == 0);
        lv_coord_t x = area.x1 + (lv_coord_t)((v10 / 50.0f) * w);
        tick_dsc.color = major ? COL_TICK_MAJOR : COL_TICK_MINOR;

        lv_coord_t inset = major ? 0 : 4;
        lv_point_t p1 = {x, (lv_coord_t)(area.y1 + inset)};
        lv_point_t p2 = {x, (lv_coord_t)(area.y2 - inset)};
        lv_draw_line(draw_ctx, &tick_dsc, &p1, &p2);
    }

    /* --- Colored fill bar --- */
    float val = bar_value[idx];
    if (val < 0) val = 0;
    if (val > 5) val = 5;
    lv_coord_t fill_w = (lv_coord_t)((val / 5.0f) * w);

    if (fill_w > 2) {
        lv_area_t fill_area;
        fill_area.x1 = area.x1;
        fill_area.y1 = area.y1;
        fill_area.x2 = area.x1 + fill_w;
        fill_area.y2 = area.y2;

        lv_draw_rect_dsc_t fill_dsc;
        lv_draw_rect_dsc_init(&fill_dsc);
        fill_dsc.bg_color  = row_color[idx];
        fill_dsc.bg_opa    = LV_OPA_COVER;
        fill_dsc.radius    = 3;

        // Subtle shadow/glow effect
        fill_dsc.shadow_width = 6;
        fill_dsc.shadow_ofs_x = 0;
        fill_dsc.shadow_ofs_y = 0;
        fill_dsc.shadow_color = row_color[idx];
        fill_dsc.shadow_opa  = (lv_opa_t)80;

        lv_draw_rect(draw_ctx, &fill_dsc, &fill_area);

        // Highlight line along top edge of fill for "glossy" look
        if (fill_w > 6) {
            lv_area_t hl_area;
            hl_area.x1 = area.x1 + 2;
            hl_area.y1 = area.y1 + 1;
            hl_area.x2 = area.x1 + fill_w - 2;
            hl_area.y2 = area.y1 + 2;

            lv_draw_rect_dsc_t hl_dsc;
            lv_draw_rect_dsc_init(&hl_dsc);
            hl_dsc.bg_color = lv_color_white();
            hl_dsc.bg_opa   = (lv_opa_t)30;
            hl_dsc.radius   = 1;
            lv_draw_rect(draw_ctx, &hl_dsc, &hl_area);
        }
    }
}

/*==========================================================================
 * Helper: create one measurement row (label + bar + value) inside a panel
 *==========================================================================*/
static void create_row(lv_obj_t *parent, int idx, const char *label_text,
                       lv_coord_t y_off)
{
    // "IN" / "OUT" label
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_add_style(lbl, &sty_row_label, 0);
    lv_obj_set_style_text_color(lbl, row_color_dim[idx], 0);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_pos(lbl, 6, y_off + BAR_Y_OFFS + 1);

    // Custom-drawn bar
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(bar, BAR_X_OFFS + 4, y_off + BAR_Y_OFFS);
    lv_obj_set_size(bar, BAR_W, BAR_H);
    lv_obj_set_user_data(bar, (void *)(intptr_t)idx);
    lv_obj_add_event_cb(bar, bar_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    bar_obj[idx] = bar;

    // Voltage readout
    lv_obj_t *vlbl = lv_label_create(parent);
    lv_obj_add_style(vlbl, &sty_value, 0);
    lv_label_set_text(vlbl, "0.00V");
    lv_obj_set_pos(vlbl, PANEL_W - BAR_VALUE_W - 4, y_off + BAR_Y_OFFS);
    value_label[idx] = vlbl;
}

/*==========================================================================
 * Helper: create one device panel (A or B)
 *==========================================================================*/
static lv_obj_t *create_device_panel(lv_obj_t *parent, const char *dev_name,
                                      lv_color_t accent, lv_coord_t y,
                                      int row0_idx, int row1_idx)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_add_style(panel, &sty_panel, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(panel, PANEL_PAD_X, y);
    lv_obj_set_size(panel, PANEL_W, PANEL_INNER_H);

    // Accent line at top of panel
    lv_obj_t *accent_line = lv_obj_create(panel);
    lv_obj_remove_style_all(accent_line);
    lv_obj_set_pos(accent_line, 0, 0);
    lv_obj_set_size(accent_line, PANEL_W, 2);
    lv_obj_set_style_bg_color(accent_line, accent, 0);
    lv_obj_set_style_bg_opa(accent_line, (lv_opa_t)180, 0);
    lv_obj_set_style_radius(accent_line, 0, 0);

    // Device name label
    lv_obj_t *dev_lbl = lv_label_create(panel);
    lv_obj_add_style(dev_lbl, &sty_device_header, 0);
    lv_obj_set_style_text_color(dev_lbl, accent, 0);
    lv_label_set_text(dev_lbl, dev_name);
    lv_obj_set_pos(dev_lbl, 6, 3);

    // Status dot (decorative)
    lv_obj_t *dot = lv_obj_create(panel);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 5, 5);
    lv_obj_set_pos(dot, PANEL_W - 14, 6);
    lv_obj_set_style_bg_color(dot, accent, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);

    // Measurement rows
    create_row(panel, row0_idx, "IN",  ROW_0_Y);
    create_row(panel, row1_idx, "OUT", ROW_1_Y);

    return panel;
}

/*==========================================================================
 * Create the header bar
 *==========================================================================*/
static void create_header(lv_obj_t *parent)
{
    // Header background
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_size(hdr, SCREEN_W, HEADER_H);
    lv_obj_set_style_bg_color(hdr, COL_HEADER_BG, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t *title = lv_label_create(hdr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title, COL_TEXT_BRIGHT, 0);
    lv_label_set_text(title, LV_SYMBOL_CHARGE " DC VOLTMETER");
    lv_obj_set_pos(title, 6, 5);

    // ESP32-S3 badge
    lv_obj_t *badge = lv_obj_create(hdr);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, 58, 16);
    lv_obj_set_pos(badge, SCREEN_W - 66, 4);
    lv_obj_set_style_bg_color(badge, COL_BADGE_BG, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge, COL_BORDER_ACCENT, 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_border_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(badge, 3, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *badge_txt = lv_label_create(badge);
    lv_obj_set_style_text_font(badge_txt, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(badge_txt, COL_BADGE_TEXT, 0);
    lv_label_set_text(badge_txt, "ESP32-S3");
    lv_obj_center(badge_txt);

    // Accent line under header
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_pos(line, 0, HEADER_H);
    lv_obj_set_size(line, SCREEN_W, 1);
    lv_obj_set_style_bg_color(line, COL_HEADER_LINE, 0);
    lv_obj_set_style_bg_opa(line, (lv_opa_t)120, 0);
}

/*==========================================================================
 * Create voltage scale at bottom
 *==========================================================================*/
static void create_scale(lv_obj_t *parent)
{
    // Scale background strip
    lv_obj_t *strip = lv_obj_create(parent);
    lv_obj_remove_style_all(strip);
    lv_obj_set_pos(strip, PANEL_PAD_X, SCALE_Y);
    lv_obj_set_size(strip, PANEL_W, SCALE_H);
    lv_obj_clear_flag(strip, LV_OBJ_FLAG_SCROLLABLE);

    // Calculate bar area within the scale (same offsets as the bars in panels)
    lv_coord_t bar_start = BAR_X_OFFS + 4;

    for (int v = 0; v <= 5; v++) {
        lv_obj_t *num = lv_label_create(strip);
        lv_obj_add_style(num, &sty_scale, 0);

        char buf[8];
        snprintf(buf, sizeof(buf), "%dV", v);
        lv_label_set_text(num, buf);

        lv_coord_t x = bar_start + (lv_coord_t)((v / 5.0f) * BAR_W);
        // Center the text on the tick position
        if (v == 0) x -= 2;
        else if (v == 5) x -= 12;
        else x -= 6;

        lv_obj_set_pos(num, x, 2);
    }
}

/*==========================================================================
 * Public API
 *==========================================================================*/
void ui_create()
{
    // Set up per-row colors
    row_color[0] = COL_DEV_A;      row_color_dim[0] = COL_DEV_A_DIM;
    row_color[1] = COL_DEV_A;      row_color_dim[1] = COL_DEV_A_DIM;
    row_color[2] = COL_DEV_B;      row_color_dim[2] = COL_DEV_B_DIM;
    row_color[3] = COL_DEV_B;      row_color_dim[3] = COL_DEV_B_DIM;

    for (int i = 0; i < 4; i++) bar_value[i] = 0;

    init_styles();

    // Screen setup
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, COL_BG_DEEP, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Build the UI
    create_header(scr);
    create_device_panel(scr, "DEVICE A", COL_DEV_A, PANEL_A_Y, 0, 1);
    create_device_panel(scr, "DEVICE B", COL_DEV_B, PANEL_B_Y, 2, 3);
    create_scale(scr);
}

void ui_update_values(float a_in, float a_out, float b_in, float b_out)
{
    float v[4] = {a_in, a_out, b_in, b_out};
    char buf[12];

    for (int i = 0; i < 4; i++) {
        if (v[i] < 0) v[i] = 0;
        if (v[i] > 5) v[i] = 5;

        bar_value[i] = v[i];
        lv_obj_invalidate(bar_obj[i]);   // trigger redraw of this bar only

        // Only update label text when the displayed value actually changes
        if (fabsf(v[i] - last_shown[i]) >= 0.005f) {
            snprintf(buf, sizeof(buf), "%.2fV", v[i]);
            lv_label_set_text(value_label[i], buf);
            last_shown[i] = v[i];
        }
    }
}
