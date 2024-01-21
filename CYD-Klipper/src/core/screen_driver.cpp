#include "screen_driver.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include "../conf/global_config.h"
#include "lvgl.h"

TAMC_GT911 tp = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

uint32_t LV_EVENT_GET_COMP_CHILD;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[TFT_WIDTH * TFT_HEIGHT / 10];

TFT_eSPI tft = TFT_eSPI();

bool isScreenInSleep = false;
lv_timer_t *screenSleepTimer;

void screen_setBrightness(byte brightness)
{
    // calculate duty, 4095 from 2 ^ 12 - 1
    uint32_t duty = (4095 / 255) * brightness;

    // write duty to LEDC
    ledcWrite(0, duty);
}

void set_screen_brightness()
{
    if (global_config.brightness < 32)
        screen_setBrightness(255);
    else
        screen_setBrightness(global_config.brightness);
}

void screen_timer_wake()
{
    lv_timer_reset(screenSleepTimer);
    isScreenInSleep = false;
    set_screen_brightness();
    set_LED_color(global_config.LEDcolors);

    // Reset cpu freq
    setCpuFrequencyMhz(CPU_FREQ_HIGH);
    Serial.printf("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz());
}

void screen_timer_sleep(lv_timer_t *timer)
{
    screen_setBrightness(0);
    isScreenInSleep = true;
    uint8_t dimVal[3] = {0, 0, 0};
    if (global_config.LEDcolors[0] > 0)
        dimVal[0] = global_config.LEDcolors[0] * 0.1;
    if (global_config.LEDcolors[1] > 0)
        dimVal[1] = global_config.LEDcolors[1] * 0.1;
    if (global_config.LEDcolors[2] > 0)
        dimVal[2] = global_config.LEDcolors[2] * 0.1;
    set_LED_color(dimVal);

    // Screen is off, no need to make the cpu run fast, the user won't notice ;)
    setCpuFrequencyMhz(CPU_FREQ_LOW);
    Serial.printf("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz());
}

void screen_timer_setup()
{
    screenSleepTimer = lv_timer_create(screen_timer_sleep, global_config.screenTimeout * 1000 * 60, NULL);
    lv_timer_pause(screenSleepTimer);
}

void screen_timer_start()
{
    lv_timer_resume(screenSleepTimer);
}

void screen_timer_stop()
{
    lv_timer_pause(screenSleepTimer);
}

void screen_timer_period(uint32_t period)
{
    lv_timer_set_period(screenSleepTimer, period);
}

void set_screen_timer_period()
{
    screen_timer_period(global_config.screenTimeout * 1000 * 60);
}

void screen_lv_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void screen_lv_touchRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    tp.read();
    if (!tp.isTouched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;
        for (int i = 0; i < tp.touches; i++)
        {
            uint16_t magicX; // fix GT911 driver - orientation and handle rotation
            uint16_t magicY;
            if (!global_config.rotateScreen)
            {
                magicY = tp.points[i].x;
                magicX = TOUCH_HEIGHT - tp.points[i].y;
            }
            else
            {
                magicY = TOUCH_WIDTH - tp.points[i].x;
                magicX = tp.points[i].y;
            }

            data->point.x = magicX;
            data->point.y = magicY;

            // Serial.print("Touch ");Serial.print(i+1);Serial.print(": ");
            // Serial.print("  x: ");Serial.print(magicX);
            // Serial.print("  y: ");Serial.print(magicY);
            // Serial.print("  size: ");Serial.println(tp.points[i].size);
        }
        lv_timer_reset(screenSleepTimer);
        if (isScreenInSleep)
            screen_timer_wake();
    }
}

void set_color_scheme()
{
    lv_disp_t *dispp = lv_disp_get_default();
    lv_color_t main_color = {0};
    COLOR_DEF color_def = color_defs[global_config.color_scheme];

    if (color_defs[global_config.color_scheme].primary_color_light > 0)
    {
        main_color = lv_palette_lighten(color_def.primary_color, color_def.primary_color_light);
    }
    else if (color_defs[global_config.color_scheme].primary_color_light < 0)
    {
        main_color = lv_palette_darken(color_def.primary_color, color_def.primary_color_light * -1);
    }
    else
    {
        main_color = lv_palette_main(color_defs[global_config.color_scheme].primary_color);
    }

    lv_theme_t *theme = lv_theme_default_init(dispp, main_color, lv_palette_main(color_def.secondary_color), !global_config.lightMode, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
}

void set_invert_display()
{
    tft.invertDisplay(global_config.invertColors);
}

void LED_init()
{
    pinMode(LED_PIN_R, OUTPUT);
    pinMode(LED_PIN_G, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
    set_LED_color(global_config.LEDcolors);
}

void set_LED_color(uint8_t rgbVal[3])
{
    analogWrite(LED_PIN_R, 255 - rgbVal[0]);
    analogWrite(LED_PIN_G, 255 - rgbVal[1]);
    analogWrite(LED_PIN_B, 255 - rgbVal[2]);
}

void screen_setup()
{
    // Initialize the touchscreen
    tp.begin();
    tp.setRotation(ROTATION_NORMAL);
    // Initialize LVGL
    lv_init();
    // Initialize the display
    tft.init();
    ledcSetup(0, 5000, 12);
    ledcAttachPin(TFT_BL, 0);
    tft.setRotation(global_config.rotateScreen ? 3 : 1);
    tft.fillScreen(TFT_BLACK);
    set_screen_brightness();
    set_invert_display();
    LED_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_WIDTH * TFT_HEIGHT / 10);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_HEIGHT;
    disp_drv.ver_res = TFT_WIDTH;
    disp_drv.flush_cb = screen_lv_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Initialize the (dummy) input device driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = screen_lv_touchRead;
    lv_indev_drv_register(&indev_drv);

    // Initialize screen dimming timer
    screen_timer_setup();
    screen_timer_start();

    // Initialize the graphics library
    LV_EVENT_GET_COMP_CHILD = lv_event_register_id();
    set_color_scheme();
}
