#pragma once
// Adapted from https://github.com/xperiments-in/xtouch/blob/main/src/devices/2.8/screen.h

#ifndef _SCREEN_DRIVER_INIT
#define _SCREEN_DRIVER_INIT

#define CPU_FREQ_HIGH 240
#define CPU_FREQ_LOW 80

#include <TAMC_GT911.h>
#include <TFT_eSPI.h>

#define TOUCH_SDA  33
#define TOUCH_SCL  32
#define TOUCH_INT 21
#define TOUCH_RST 25
#define TOUCH_WIDTH 320
#define TOUCH_HEIGHT 480

#define LED_PIN_R 4
#define LED_PIN_G 16
#define LED_PIN_B 17

// TS_Point touchscreen_point();
void screen_setBrightness(byte brightness);
void screen_timer_setup();
void screen_timer_start();
void screen_timer_stop();
void screen_timer_period(uint32_t period);
void set_color_scheme();
void screen_setup();
void set_invert_display();
void screen_timer_wake();
void set_screen_timer_period();
void set_screen_brightness();
void LED_init();
void set_LED_color(uint8_t rgbValue[3]);

extern TFT_eSPI tft;

#endif // _SCREEN_DRIVER_INIT
