#include "epaper.hpp"
#include <WiFi.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

#include "resources/weather-icons.hpp"
#include "resources/fontello10pt7b.hpp"
#include <U8g2_for_Adafruit_GFX.h>
#include "settings.hpp"
#include "battery.hpp"
#include "lang.hpp"
#include "locallog.hpp"

#define SPEAKER_ICON "0"
#define SELECTED_ICON "1"
#define CHARGING_ICON "2"
#define STATION_ICON "4"
#define WIFI_ICON_1 "9"
#define WIFI_ICON_2 "8"
#define WIFI_ICON_3 "7"
#define WIFI_ICON_4 "6"
#define WIFI_ICON_5 "5"

#define rssi_x 245
#define rssi_y 10
#define battery_x 177
#define battery_y 13
#define time_x 175
#define time_y 55

extern Settings settings;
struct DateTimeInfo dtInfo;
int stream_title_xpos;
int stream_title_width;
bool slide_stream_title;

//2.7″ E-Paper module 264*176 Pixels
GxEPD2_BW<GxEPD2_270, GxEPD2_270::HEIGHT> display(GxEPD2_270(EPAPER_CS, EPAPER_DC, EPAPER_RST, EPAPER_BUSY));
//GxEPD2_3C<GxEPD2_270c, GxEPD2_270c::HEIGHT> display(GxEPD2_270c(/*CS=*/5, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
// Using fonts:
// u8g2_font_helvB08_tf
// u8g2_font_helvB10_tf
// u8g2_font_helvB12_tf
// u8g2_font_helvB14_tf
// u8g2_font_helvB18_tf
// u8g2_font_helvB24_tf

#define SCREEN_WIDTH   display.width()
#define SCREEN_HEIGHT  display.height()

enum alignmentType {LEFT, RIGHT, CENTER};

/*
String get_day_of_week(int wday)
{
    String days[7] = {SUNDAY, MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY};
    return days[wday];
}
*/

void refresh_date_time_now()
{
    struct tm now2;
    char ts_str[10];
    char day_output[15];

    dtInfo.time = "--:--";
    dtInfo.weekDay = weekday_DD[0];
    dtInfo.date = "01-Jan-1970";

    llog_d("Refresh date time now");

    if (getLocalTime(&now2)) //max 10 ms
    {
        strftime(ts_str, sizeof(ts_str), "%H:%M", &now2);
      
        dtInfo.time = String(ts_str);
        dtInfo.weekDay = weekday_DD[now2.tm_wday];

        sprintf(day_output, "%02u-%s-%04u", now2.tm_mday, month_M[now2.tm_mon], (now2.tm_year) + 1900);
        dtInfo.date = String(day_output);
    }

    llog_d("Current Date: %s, Time: %s, Day: %s", dtInfo.date, dtInfo.time, dtInfo.weekDay);
}

void drawString(int x, int y, String text, alignmentType alignment) {
  int16_t  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  uint16_t w, h;
  display.setTextWrap(false);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT)  x = x - w;
  if (alignment == CENTER) x = x - w / 2;
  u8g2Fonts.setCursor(x, y + h);
  u8g2Fonts.print(text);
}

void epaper_draw_time(int x, int y)
{
    refresh_date_time_now();

    llog_d("Draw time: %s", dtInfo.time);

    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(time_x, time_y);
    display.print(dtInfo.time);

}

void epaper_redraw_time()
{
    #define time_h 27
    #define time_w 85
    llog_d("ReDraw time");

    display.setPartialWindow(time_x, time_y-time_h+1, time_w, time_h);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        epaper_draw_time(time_x, time_y);
    } while (display.nextPage());
    display.powerOff();
}

/*
void epaper_draw_heading_section() {

  dbgPrintln("City: " + settings.City + ", date: " + date_str + ", time: " + time_str);

  u8g2Fonts.setFont(u8g2_font_helvB08_tf);
  //display.drawRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,GxEPD_BLACK);
  drawString(0, 15, settings.City, LEFT);
  drawString(0, 1, time_str, LEFT);
  drawString(SCREEN_WIDTH, 1, date_str, RIGHT);
  display.drawLine(0, 11, SCREEN_WIDTH, 11, GxEPD_BLACK);

  DrawBattery(55, 12);
}




void subrutine_date(String date, String dayOfWeek)
{
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(5, 55);
    display.print(date);
    display.setCursor(5, 75);
    display.print(dayOfWeek);
}

void set_epaper_date(String date, String dayOfWeek)
{
    display.setPartialWindow(0, 45, 100, 35);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_date(date, dayOfWeek);
    } while (display.nextPage());
    display.powerOff();
}
*/
void subrutine_meteo(String temperature, char icon)
{
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&Meteocons_Regular_35);
    display.setCursor(109, 32);
    display.write(39);
    display.setFont(&FreeSansBold18pt7b);
    display.print(temperature);
    display.setFont(&Meteocons_Regular_35);
    display.write(42);
    display.setCursor(150, 78);
    display.write(icon);
}

void set_epaper_meteo(String temperature, char icon)
{
    display.setPartialWindow(109, 0, display.width() - 109, 80);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_meteo(temperature, icon);
    } while (display.nextPage());
    display.powerOff();
}

void epaper_draw_station(int x, int y, String station)
{
    llog_e("Draw station (x %d, y %d): %s", x, y, station.c_str());

    display.setTextWrap(false);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(x, y);
    display.print(station);
}

void epaper_redraw_station(String station, bool _resetPosition)
{
    String _sttream_title = String(station);
    //llog_e("ReDraw station: %s", station.c_str());

    if (_resetPosition)
    {
        stream_title_xpos = 0;
        slide_stream_title = false;

        int16_t  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
        uint16_t w, h;
        display.setTextWrap(false);
        display.setFont(&FreeSansBold9pt7b);
        display.setTextWrap(false);
        display.getTextBounds(station, 0, 50, &x1, &y1, &w, &h);
        stream_title_width = w;
        if (w > SCREEN_WIDTH)
        {
            slide_stream_title = true;
        }
    }    

    if (slide_stream_title)
    {
        _sttream_title = station + ".     " + station;
    }
    else if (!_resetPosition)
    {
        return; //draw only once
    }

    display.setPartialWindow(0, 128, display.width(), display.height() - 125);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        epaper_draw_station(-stream_title_xpos, 140, _sttream_title);
    } while (display.nextPage());
    display.powerOff();

    if (slide_stream_title)
    {
        stream_title_xpos += 5;
        if (stream_title_xpos > stream_title_width + 28)
        {
            stream_title_xpos = 0;
        }
    }
}

void epaper_draw_rssi(int x, int y, int rssi) {

    llog_d("Draw RSSI: %d", rssi);

    int WIFIsignal = 0;
    int xpos = 0;
    for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 15) {
        if (_rssi <= -40)  WIFIsignal = 10; //            <-20dbm displays 5-bars
        if (_rssi <= -55)  WIFIsignal = 8; //  -40dbm to  -21dbm displays 4-bars
        if (_rssi <= -70)  WIFIsignal = 6; //  -60dbm to  -41dbm displays 3-bars
        if (_rssi <= -85)  WIFIsignal = 4; //  -80dbm to  -61dbm displays 2-bars
        if (_rssi <= -100) WIFIsignal = 2;  // -100dbm to  -81dbm displays 1-bar
        display.fillRect(x + xpos * 4, y - WIFIsignal, 3, WIFIsignal, GxEPD_BLACK);
        xpos++;
    }
}

void epaper_redraw_rssi(int rssi)
{
    #define rssi_w 20
    #define rssi_h 10
    llog_d("ReDraw RSSI: %d", rssi);

    display.setPartialWindow(rssi_x, rssi_y-rssi_h, rssi_w, rssi_h);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        epaper_draw_rssi(rssi_x, rssi_y, rssi);
        display.drawFastHLine(rssi_x, 13, rssi_w, GxEPD_BLACK);
    } while (display.nextPage());
    display.powerOff();
}

/*
void subrutine_battery(uint8_t percentage)
{
    display.drawRect(5, 98, 40, 15, GxEPD_BLACK);
    display.fillRect(45, 102, 3, 7, GxEPD_BLACK);
    display.fillRect(5, 98, (int16_t)(40 * percentage / 100), 15, GxEPD_BLACK);
}
*/

void epaper_draw_battery(int x, int y, uint8_t percentage) {

    int shift_right = percentage >= 100? 0 : 5;

    display.drawRect(x + 15 + shift_right, y - 12, 19, 10, GxEPD_BLACK);
    display.fillRect(x + 34 + shift_right, y - 10, 2, 5, GxEPD_BLACK);
    display.fillRect(x + 17 + shift_right, y - 10, 15 * percentage / 100.0, 6, GxEPD_BLACK);

    u8g2Fonts.setCursor(x + 38 + shift_right, y-3);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);   
    u8g2Fonts.print(String(percentage) + "%");
  
}

void epaper_redraw_battery(uint8_t percentage)
{
    int shift_right = percentage >= 100? 0 : 5;
    display.setPartialWindow(battery_x + shift_right, 0, 65, 9);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        epaper_draw_battery(battery_x, battery_y, percentage);
        display.drawFastHLine(battery_x + shift_right, battery_y, 65, GxEPD_BLACK);
    } while (display.nextPage());
    display.powerOff();
}

void subrutine_volume(uint8_t value)
{
    display.setFont(&fontello10pt7b);
    display.setCursor(144, 111);
    display.print(SPEAKER_ICON);
    display.setFont(&FreeSans9pt7b);
    display.printf("%d", value);
}

void set_epaper_volume(uint8_t value)
{
    display.setPartialWindow(144, 97, 70, 17);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_volume(value);
    } while (display.nextPage());
    display.powerOff();
}

void subrutine_station_number(uint8_t value)
{
    display.setFont(&fontello10pt7b);
    display.setCursor(215, 111);
    display.print(STATION_ICON);
    display.setFont(&FreeSans9pt7b);
    display.printf(" %d", value);
}

void set_epaper_station_number(uint8_t value)
{
    display.setPartialWindow(215, 97, 70, 17);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_station_number(value + 1);
    } while (display.nextPage());
    display.powerOff();
}

void subrutine_cursor_volume(bool volume_mode)
{
    display.setFont(&fontello10pt7b);
    display.setCursor(134, 112);
    display.setTextColor(volume_mode ? GxEPD_BLACK : GxEPD_WHITE);
    display.print(SELECTED_ICON);
    display.setTextColor(GxEPD_BLACK);
}

void subrutine_cursor_station(bool volume_mode)
{
    display.setFont(&fontello10pt7b);
    display.setCursor(204, 112);
    display.setTextColor(volume_mode ? GxEPD_WHITE : GxEPD_BLACK);
    display.print(SELECTED_ICON);
    display.setTextColor(GxEPD_BLACK);
}

void set_epaper_cursor(bool volume_mode)
{
    display.setPartialWindow(134, 97, 12, 17);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_cursor_volume(volume_mode);
    } while (display.nextPage());

    display.setPartialWindow(204, 97, 12, 17);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_cursor_station(volume_mode);
    } while (display.nextPage());
    display.powerOff();
}

void init_display()
{
    SPI.end();
    SPI.begin(18, 12, 23, 15);
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.refresh();

    u8g2Fonts.begin(display);                  // connect u8g2 procedures to Adafruit GFX
    u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
    u8g2Fonts.setFontDirection(0);             // left to right (this is default)
    u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
    u8g2Fonts.setFont(u8g2_font_helvB10_tf);   // Explore u8g2 fonts from here: https://github.com/olikraus/u8g2/wiki/fntlistall 
}

void logo_screen(String message)
{
    display.setFont(&FreeSansBold24pt7b);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds("iArradio", 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 3) - tby;
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        u8g2Fonts.setCursor(0, 12);
        u8g2Fonts.setFont(u8g2_font_helvB10_tf);
        u8g2Fonts.print(settings.City);

        u8g2Fonts.setCursor(110, 10);
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);   
        u8g2Fonts.print(dtInfo.date);

        epaper_draw_battery(battery_x + 23, battery_y, get_battery_capacity());

        display.setCursor(x, y);
        display.print("iArradio");
        //display.setCursor(0, 120);
        u8g2Fonts.setCursor(0, 120);
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);
        u8g2Fonts.print(message);
        //display.setFont(FreeSans9pt7b);
        //display.print(message);
    } while (display.nextPage());
    display.powerOff();
}

void main_interface()
{
    refresh_date_time_now();

    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.drawFastHLine(0, 124, 264, GxEPD_BLACK);
        display.drawFastHLine(0, 13, 264, GxEPD_BLACK);

        u8g2Fonts.setCursor(0, 12);
        u8g2Fonts.setFont(u8g2_font_helvB10_tf);
        u8g2Fonts.print(settings.City);

        u8g2Fonts.setCursor(110, 10);
        u8g2Fonts.setFont(u8g2_font_helvB08_tf);   
        u8g2Fonts.print(dtInfo.date);

        epaper_draw_battery(battery_x, battery_y, get_battery_capacity());
        epaper_draw_rssi(rssi_x, rssi_y, WiFi.RSSI());
        epaper_draw_time(time_x, time_y);

        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(time_x, time_y + 20);
        display.print(dtInfo.weekDay);

        //set_epaper_wifi_signal(245, 10, -60, true);

        //display.drawFastVLine(108, 0, 85, GxEPD_BLACK);
        //subrutine_battery(0);

    } while (display.nextPage());

    display.powerOff();
}
