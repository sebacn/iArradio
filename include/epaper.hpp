#ifndef _epaper_h
#define _epaper_h

#include <Arduino.h>
#include <SPI.h>
#define ENABLE_GxEPD2_GFX 0
#define DISABLE_DIAGNOSTIC_OUTPUT
#include <GxEPD2_BW.h>
// #include <GxEPD2_3C.h>
#include "hal.hpp"

struct DateTimeInfo {
    String time;
    String date;
    String weekDay;
};

void init_display();
void weathericonsTest();
void main_interface();
void logo_screen(String message);
//void set_epaper_time(String time);
//void set_epaper_date(String date, String dayOfWeek);
void set_epaper_meteo(String temperature, char icon);
//void set_epaper_station(String station, bool _resetPosition);
//void set_epaper_battery(uint8_t percentage);
//void subrutine_time(String time);
void subrutine_meteo(String temperature, char icon);
//void subrutine_date(String date, String dayOfWeek);
//void subrutine_station(String station);
//void subrutine_battery(uint8_t percentage);
void subrutine_volume(uint8_t value);
void set_epaper_volume(uint8_t value);
void subrutine_cursor_volume(bool volume_mode);
void subrutine_cursor_station(bool volume_mode);
void set_epaper_cursor(bool volume_mode);
void set_epaper_station_number(uint8_t value);
void subrutine_station_number(uint8_t value);
//void set_epaper_wifi_signal(uint8_t rssi);
//void subrutine_wifi_signal(uint8_t rssi);
//void epaper_draw_heading_section();
//void set_epaper_wifi_signal(int x, int y, int rssi);

void epaper_draw_battery(int x, int y, uint8_t percentage);
void epaper_redraw_battery(uint8_t percentage);

void epaper_draw_rssi(int x, int y, int rssi);
void epaper_redraw_rssi(int rssi);

void epaper_draw_time(int x, int y);
void epaper_redraw_time();

void epaper_draw_station(int x, int y, String station);
void epaper_redraw_station(String station, bool _resetPosition);

#endif