/*******************************************************************************
 * display_setup.cpp
 *
 * Display init sequence is taken directly from the Waveshare
 * 02_gfx_helloworld.ino reference (ST7789, SPI, IPS panel).
 * LVGL v8.3 draw-driver is wired on top of Arduino_GFX.
 ******************************************************************************/
#include "display_setup.h"
#include "config.h"
#include <lvgl.h>

/*--------------------------------------------------------------------------
 * Arduino_GFX bus & display — IDENTICAL to the helloworld sample
 *--------------------------------------------------------------------------*/
Arduino_DataBus *bus = new Arduino_ESP32SPI(
    LCD_PIN_DC /* DC */, LCD_PIN_CS /* CS */,
    LCD_PIN_SCLK /* SCK */, LCD_PIN_MOSI /* MOSI */, LCD_PIN_MISO /* MISO */);

Arduino_GFX *gfx = new Arduino_ST7789(
    bus, LCD_PIN_RST /* RST */, LCD_ROTATION /* rotation */, true /* IPS */,
    LCD_H_RES_NATIVE /* width */, LCD_V_RES_NATIVE /* height */);

/*--------------------------------------------------------------------------
 * LVGL draw buffers  (partial-render: a handful of scanlines)
 *--------------------------------------------------------------------------*/
#define LVGL_BUF_LINES 40

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_color_t *buf2;
static lv_disp_drv_t disp_drv;

/*--------------------------------------------------------------------------
 * Flush callback — pushes LVGL's rendered rectangle to the ST7789 via GFX
 *--------------------------------------------------------------------------*/
static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area,
                           lv_color_t *color_p)
{
    uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1,
                               (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1,
                             (uint16_t *)&color_p->full, w, h);
#endif

    lv_disp_flush_ready(drv);
}

/*--------------------------------------------------------------------------
 * Public init  —  display hardware + LVGL display driver
 *--------------------------------------------------------------------------*/
void display_and_lvgl_init()
{
    /* --- Display init (matches helloworld exactly) --- */
    if (!gfx->begin()) {
        Serial.println("gfx->begin() failed!");
    }
    gfx->fillScreen(0x0000);

    pinMode(LCD_PIN_BL, OUTPUT);
    digitalWrite(LCD_PIN_BL, HIGH);

    /* --- LVGL init --- */
    lv_init();

    size_t buf_px = (size_t)SCREEN_W * LVGL_BUF_LINES;

    buf1 = (lv_color_t *)heap_caps_malloc(
        buf_px * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    buf2 = (lv_color_t *)heap_caps_malloc(
        buf_px * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

    if (!buf1 || !buf2) {
        Serial.println("LVGL DMA buf alloc failed, trying PSRAM...");
        if (!buf1)
            buf1 = (lv_color_t *)heap_caps_malloc(
                buf_px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
        if (!buf2)
            buf2 = (lv_color_t *)heap_caps_malloc(
                buf_px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    }

    if (!buf1 || !buf2) {
        Serial.println("FATAL: Could not allocate LVGL draw buffers!");
        while (1) delay(1000);
    }

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_px);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = SCREEN_W;
    disp_drv.ver_res  = SCREEN_H;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}
