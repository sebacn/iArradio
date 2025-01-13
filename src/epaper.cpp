#include "epaper.hpp"
#include <WiFi.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "resources/weather-icons.hpp"
#include "resources/fontello10pt7b.hpp"
#include <U8g2_for_Adafruit_GFX.h>
#include "settings.hpp"

#define SPEAKER_ICON "0"
#define SELECTED_ICON "1"
#define CHARGING_ICON "2"
#define STATION_ICON "4"
#define WIFI_ICON_1 "9"
#define WIFI_ICON_2 "8"
#define WIFI_ICON_3 "7"
#define WIFI_ICON_4 "6"
#define WIFI_ICON_5 "5"

extern Settings settings;

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

extern String date_str;
extern String time_str;

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

void DrawBattery(int x, int y) {
  uint8_t percentage = 100;
  float voltage = analogRead(35) / 4096.0 * 7.46;
  if (voltage > 1 ) { // Only display if there is a valid reading
    dbgPrintln("Voltage = " + String(voltage));
    percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
    if (voltage >= 4.20) percentage = 100;
    if (voltage <= 3.50) percentage = 0;
    display.drawRect(x + 15, y - 12, 19, 10, GxEPD_BLACK);
    display.fillRect(x + 34, y - 10, 2, 5, GxEPD_BLACK);
    display.fillRect(x + 17, y - 10, 15 * percentage / 100.0, 6, GxEPD_BLACK);

    u8g2Fonts.setCursor(x+40, y-3);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);   
    u8g2Fonts.print(String(percentage) + "%");
 
    //drawString(x + 40, y, String(percentage) + "%", RIGHT);
    //drawString(x + 13, y + 5,  String(voltage, 2) + "v", CENTER);
  }
}

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

/*
void subrutine_time(String time)
{
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(10, 32);
    display.print(time);
}

void set_epaper_time(String time)
{
    display.setPartialWindow(10, 6, 85, 27);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_time(time);
    } while (display.nextPage());
    display.powerOff();
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

void subrutine_station(String station)
{
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(0, 140);
    display.print(station);
}

void set_epaper_station(String station)
{
    display.setPartialWindow(0, 128, display.width(), display.height() - 125);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_station(station);
    } while (display.nextPage());
    display.powerOff();
}

void DrawRSSI(int x, int y, int rssi) {

    dbgPrintln("DrawRSSI");

    int WIFIsignal = 0;
    int xpos = 0;
    for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
        if (_rssi <= -20)  WIFIsignal = 10; //            <-20dbm displays 5-bars
        if (_rssi <= -40)  WIFIsignal = 8; //  -40dbm to  -21dbm displays 4-bars
        if (_rssi <= -60)  WIFIsignal = 6; //  -60dbm to  -41dbm displays 3-bars
        if (_rssi <= -80)  WIFIsignal = 4; //  -80dbm to  -61dbm displays 2-bars
        if (_rssi <= -100) WIFIsignal = 2;  // -100dbm to  -81dbm displays 1-bar
        display.fillRect(x + xpos * 4, y - WIFIsignal, 3, WIFIsignal, GxEPD_BLACK);
        xpos++;
    }

    display.drawFastHLine(x, 13, 264-y, GxEPD_BLACK);
}

void set_epaper_wifi_signal(int x, int y, int rssi)
{
    dbgPrintln("set_epaper_wifi_signal");
    display.setPartialWindow(x, y-10, 20, 10);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        DrawRSSI(x, y, rssi);
    } while (display.nextPage());
    display.powerOff();
}

void subrutine_battery(uint8_t percentage)
{
    display.drawRect(5, 98, 40, 15, GxEPD_BLACK);
    display.fillRect(45, 102, 3, 7, GxEPD_BLACK);
    display.fillRect(5, 98, (int16_t)(40 * percentage / 100), 15, GxEPD_BLACK);
}

void set_epaper_battery(uint8_t percentage)
{
    display.setPartialWindow(0, 97, 42, 17);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_battery(percentage);
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
        u8g2Fonts.print(date_str);

        DrawBattery(170, 13);

        //set_epaper_wifi_signal(245, 10, WiFi.RSSI());

        

        //display.drawFastVLine(108, 0, 85, GxEPD_BLACK);
        subrutine_battery(0);

    } while (display.nextPage());
    display.powerOff();
}
