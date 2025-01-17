#include "controls.hpp"
//#include <Arduino.h>
#include "driver/rtc_io.h"
#include "locallog.hpp"

bool power_off_on;

void power_off()
{
    llog_d("Power off");
    power_off_on = true;

    set_updating(true);
    audio_stop();
    logo_screen(POWEROFF);
    sleep(2);    
    rtc_gpio_pulldown_dis((gpio_num_t)HOME_BUTTON);
    rtc_gpio_pullup_en((gpio_num_t)HOME_BUTTON);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)HOME_BUTTON, 0);
    esp_deep_sleep_start();
}

void low_battery()
{
    llog_d("Low battery");
    power_off_on = true;
    
    set_updating(true);
    audio_stop();
    logo_screen(LOWBATT);
    sleep(2);
    esp_deep_sleep_start();
}