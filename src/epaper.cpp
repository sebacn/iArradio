#include "epaper.hpp"

//#include <WiFi.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

#include <Fonts/FreeMonoBold9pt7b.h>

#include "resources/weather-icons.hpp"
#include "resources/fontello10pt7b.hpp"
#include <U8g2_for_Adafruit_GFX.h>
#include "settings.hpp"
#include "battery.hpp"
#include "lang.hpp"
#include "locallog.hpp"
//#include "img_64bw.hpp"
#include "img_115bw.hpp"
#include "img_tst60px2bpp.h"
#include "img_tst60px4bpp.h"
#include "img_tst60px8bpp.h"
#include "img_tst115px2bpp.h"
#include "img_tst115px8bpp.h"
#include "img_tst150px2bpp.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"
//#include <TJpg_Decoder.h>
#include "JPEGDEC.h"
#include "imagerender.hpp"

//JPEGDEC jpeg;
//#define JPEG_CPY_FRAMEBUFFER false

uint32_t img_buf_pos = 0;
uint32_t time_decomp = 0;
uint32_t time_render = 0;
// Dither space allocation
//uint8_t * dither_space;

// Affects the gamma to calculate gray (lower is darker/higher contrast)
// Nice test values: 0.9 1.2 1.4 higher and is too bright
//double gamma_value = 0.7;
// Internal array for gamma grayscale
//uint8_t gamme_curve[256];
//uint8_t *source_buf;


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

SPIClass SDSPI(VSPI);

const char* card_types[] = {"CARD_NONE", "CARD_MMC", "CARD_SD", "CARD_SDHC", "CARD_UNKNOWN"};

//void drawBitmapFromSD(const char *filename, int16_t x, int16_t y, bool with_color);
//void drawBitmapFromSD(const char *filename, int16_t x, int16_t y, bool with_color = true);
//void drawBitmapFromSD_V2(const char *filename, int16_t x, int16_t y, bool with_color);
//void drawBitmapFromSD_V2(const char *filename, int16_t x, int16_t y, bool with_color = true);
//void showPartialUpdate();

extern Settings settings;
struct DateTimeInfo dtInfo;
int stream_title_xpos;
int stream_title_width;
bool slide_stream_title;
//uint8_t *jpg_bitmap;
//uint16_t jpg_ptr = 0;

//2.7″ E-Paper module 264*176 Pixels
// #include <GxGDEW027W3/GxGDEW027W3.h>      // 2.7" b/w
GxEPD2_4G_BW<GxEPD2_270, GxEPD2_270::HEIGHT> display(GxEPD2_270(EPAPER_CS, EPAPER_DC, EPAPER_RST, EPAPER_BUSY));
//GxEPD2_4G_4G<GxEPD2_270, GxEPD2_270::HEIGHT> display(GxEPD2_270(EPAPER_CS, EPAPER_DC, EPAPER_RST, EPAPER_BUSY)); // GDEW027W3 176x264, EK79652 (IL91874), (WFI0190CZ22)
//GxEPD2_3C<GxEPD2_270c, GxEPD2_270c::HEIGHT> display(GxEPD2_270c(/*CS=*/5, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
// Using fonts:
// u8g2_font_helvB08_tf
// u8g2_font_helvB10_tf
// u8g2_font_helvB12_tf
// u8g2_font_helvB14_tf
// u8g2_font_helvB18_tf
// u8g2_font_helvB24_tf

#define SCREEN_WIDTH   display.width() //264
#define SCREEN_HEIGHT  display.height() //176

enum alignmentType {LEFT, RIGHT, CENTER};

//uint8_t * jpg_bitmap_2ppb;

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
*/

void epaper_draw_stream_title(int x, int y, String stream_title)
{
    llog_e("Draw stream_title (x %d, y %d): %s", x, y, stream_title.c_str());

    display.setTextWrap(false);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(x, y);
    display.print(stream_title);
}

void epaper_redraw_stream_title(String stream_title, bool _resetPosition)
{
    #define stream_title_x 0
    #define stream_title_y 173

    String _sttream_title = String(stream_title);
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
        display.getTextBounds(stream_title, 0, 50, &x1, &y1, &w, &h);
        stream_title_width = w;
        if (w > SCREEN_WIDTH - stream_title_x)
        {
            slide_stream_title = true;
        }
    }    

    if (slide_stream_title)
    {
        _sttream_title = stream_title + ".     " + stream_title;
    }
    else if (!_resetPosition)
    {
        return; //draw only once
    }

    display.setPartialWindow(stream_title_x, stream_title_y - 16 + 3, display.width() - stream_title_x, 16);
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        epaper_draw_stream_title(stream_title_x - stream_title_xpos, stream_title_y, _sttream_title);
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


void epaper_draw_station_title(int x, int y, String station)
{
    llog_e("Draw station title (x %d, y %d): %s", x, y, station.c_str());

    subrutine_station_number(x, y, 0);

    display.setTextWrap(false);
    display.setTextColor(GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_helvB12_tf); 
    u8g2Fonts.setCursor(x+20, y);      
    u8g2Fonts.print(station);

/*
    display.setTextWrap(false);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(x, y);
    display.print(station);
    */
}

void epaper_redraw_station_title(String station)
{
    #define station_title_x 150 //half w
    #define station_title_y 155
    
    display.setPartialWindow(station_title_x, station_title_y-15, display.width()-station_title_x, 15);
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        epaper_draw_station_title(station_title_x, station_title_y, station);
    } while (display.nextPage());
    display.powerOff();
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
    display.firstPage();
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
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        epaper_draw_battery(battery_x, battery_y, percentage);
        display.drawFastHLine(battery_x + shift_right, battery_y, 65, GxEPD_BLACK);
    } while (display.nextPage());
    display.powerOff();
}

void subrutine_volume(int x, int y, uint8_t value)
{
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&fontello10pt7b);
    display.setCursor(x, y);
    display.print(SPEAKER_ICON);
    display.setFont(&FreeSans9pt7b);
    display.printf("%d", value);
}

void set_epaper_volume(uint8_t value)
{
    #define volume_x 165
    #define volume_y 130
    #define volume_h 17

    display.setPartialWindow(volume_x, volume_y - volume_h + 3, 70, volume_h);   
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_volume(volume_x, volume_y, value);
    } while (display.nextPage());
    display.powerOff();
}

void subrutine_station_number(int x, int y, uint8_t value)
{
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&fontello10pt7b);
    display.setCursor(x, y);
    display.print(STATION_ICON);
    //display.setFont(&FreeSans9pt7b);
    //display.printf(" %d", value);
}

/*
void set_epaper_station_number(uint8_t value)
{
    #define stnum_x 144
    #define stnum_y 60
    #define stnum_h 17

    display.setPartialWindow(stnum_x, stnum_y - stnum_h + 3, 70, stnum_h);
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_station_number(stnum_x, stnum_y, value + 1);
    } while (display.nextPage());
    display.powerOff();
}
*/

/*
cursor removed - used volume knob
void subrutine_cursor_volume(int x, int y, bool volume_mode)
{
    llog_d("Draw cursor_volume (mode %d)", volume_mode);

    display.setFont(&fontello10pt7b);
    display.setCursor(x, y);
    display.setTextColor(volume_mode ? GxEPD_BLACK : GxEPD_WHITE);
    display.print(SELECTED_ICON);
    display.setTextColor(GxEPD_BLACK);
}

void subrutine_cursor_station(int x, int y, bool volume_mode)
{
    llog_d("Draw cursor_station (mode %d)", volume_mode);

    display.setFont(&fontello10pt7b);
    display.setCursor(x, y);
    display.setTextColor(volume_mode ? GxEPD_WHITE : GxEPD_BLACK);
    display.print(SELECTED_ICON);
    display.setTextColor(GxEPD_BLACK);
}

void set_epaper_cursor(bool volume_mode)
{
    display.firstPage();
    display.setPartialWindow(volume_x-10, volume_y-volume_h+3, 12, volume_h);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_cursor_volume(volume_x-10, volume_y, volume_mode);
    } while (display.nextPage());

    display.firstPage();
    display.setPartialWindow(stnum_x-10, stnum_y-stnum_h+3, 12, stnum_h);
    do
    {
        display.fillScreen(GxEPD_WHITE);
        subrutine_cursor_station(stnum_x-10, stnum_y, volume_mode);
    } while (display.nextPage());
    display.powerOff();
}
*/


/*
const char HelloWorld[] = "Hello World!";

void helloWorld()
{
  Serial.println("helloWorld");
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  llog_d("11111");
  display.setFullWindow();
  display.firstPage();
  llog_d("22222");
  do
  {
    llog_d("333333");
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(HelloWorld);
    llog_d("44444");
  }
  while (display.nextPage());
  Serial.println("helloWorld done");
}
*/
void drawBitmaps4g176x264()
{
  if ((display.epd2.WIDTH >= 176) && (display.epd2.HEIGHT >= 264))
  {
llog_d("drawBitmaps4g176x264 OK");

    display.setFullWindow();
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.drawGreyPixmap(Bitmap4g176x264_1, 4, 0, 0, 176, 264);
    }
    while (display.nextPage());
    delay(2000);
    display.epd2.drawImage_4G(Bitmap4g176x264_1, 4, 0, 0, 176, 264, false, false, true);
    delay(2000);

    display.epd2.drawImage_4G(Bi115bw, 8, 0, 0, 115, 115, false, false, true);

    delay(5000);
  }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  llog_d("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    llog_d("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    llog_d("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      llog_d("DIR : %s", file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      llog_d("FILE: %s, SIZE: %d", file.name(), file.size());
    }
    file = root.openNextFile();
  }
}


//bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
//{
   // Stop further decoding as image is running off bottom of screen
//  if ( y >= display.epd2.HEIGHT ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  //tft.pushImage(x, y, w, h, bitmap);
  //

/*
  int16_t buff_len = w*h;

  for (int16_t idx = 0; idx < buff_len; idx++)
  {
    jpg_bitmap[x*y + idx] = (uint8_t)bitmap[idx];
  }
  */

  //memcpy(jpg_bitmap, bitmap, buff_len);
  //llog_i("tft_output x:%d, y:%d, w:%d, h:%d draw(%d,%d,%d,%d)", x, y, w, h, 
  //y*120 + x, y*120 + x + 7, (y+7)*120 + x, (y+7)*120 + x + 7);

/*
  int16_t idx = 0;
  for (int16_t hh = 0; hh < h; hh++)
  {
    for (int16_t ww = 0; ww < w; ww++)
    {
      jpg_bitmap[(y+hh)*120 + x + ww] = (uint8_t)bitmap[idx]; //hh*w + ww]; 
      idx++;
    }
  }
*/


  //ttestt = (ttestt == 0xFF)? 0x00 : 0xFF;

/*
  display.setPartialWindow(x, y+11, w, h);
//display.firstPage();
  do
  {
    //display.fillScreen(GxEPD_WHITE);
    display.drawGreyPixmap((uint8_t *) bitmap, 8, x, y, w, h);
  }
  while (display.nextPage());
*/
  

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  //return 1;
//}

void init_display()
{
    llog_d("Init eink display");

    //delay(35000);
    SPI.end();
    SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI); //, 15); // SPI.begin(18, 12, 23, 15);
    display.init(115200);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    //display.refresh();

    u8g2Fonts.begin(display);                  // connect u8g2 procedures to Adafruit GFX
    u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
    u8g2Fonts.setFontDirection(0);             // left to right (this is default)
    u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
    u8g2Fonts.setFont(u8g2_font_helvB10_tf);   // Explore u8g2 fonts from here: https://github.com/olikraus/u8g2/wiki/fntlistall 


    llog_d("Eink init OK, pages %d, pageHeight %d", display.pages(), display.pageHeight());


    llog_i("SD Initializing card...");
    
    SDSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI);
    if (!SD.begin(SDCARD_CS, SDSPI)) {
        llog_e("SD initialization failed!");
        while (1);
    }

    llog_i("SD initialization done.");

    uint8_t cardType = SD.cardType();

    llog_d("SD Card Type: %s", card_types[cardType > 4? 4 : cardType]);

    if(cardType >= 4){
        llog_e("SD Card not identified - UNKNOWN");
        while (1);
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    llog_i("SD Card Size: %lluMB\n", cardSize);
    
    listDir(SD, "/", 3);

delay(2000);


    display.setPartialWindow(20, 20, 60, 60);
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.drawGreyPixmap(img_tst60px2bpp, 2, 20, 20, 60, 60);
    }
    while (display.nextPage_4G());
    display.powerOff();
    llog_i("drawGreyPixmap(img_tst60px2bpp 2");
    delay(7000);

 
    display.setPartialWindow(20, 20, 115, 115);
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.drawGreyPixmap(img_tst115px2bpp, 2, 20, 20, 115, 115);
    }
    while (display.nextPage_4G());
    display.powerOff();
    llog_i("drawGreyPixmap(img_tst115px2bpp, 2");
    delay(7000);
  

    display.setPartialWindow(15, 15, 150, 150);
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.drawGreyPixmap(img_tst150px2bpp, 2, 15, 15, 150, 150);
    }
    while (display.nextPage_4G());
    display.powerOff();
    llog_i("drawGreyPixmap(img_tst150px2bpp, 2");
    delay(7000);



/*
    display.setRotation(1);




    display.epd2.drawImage_4G(Bitmap4g176x264_1, 4, 0, 0, 176, 264, false, false, true);
    llog_i("drawImage_4G Bitmap4g176x264_1");
    delay(5000);

    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.drawGreyPixmap(img_tst60px2bpp, 2, 20, 20, 60, 60);
    }
    while (display.nextPage());
    llog_i("drawGreyPixmap(img_tst60px2bpp 2");
    delay(7000);

    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.drawGreyPixmap(img_tst115px2bpp, 2, 20, 20, 115, 115);
    }
    while (display.nextPage());
    llog_i("drawGreyPixmap(img_tst115px2bpp, 2");
    delay(7000);

    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.drawGreyPixmap(img_tst150px2bpp, 2, 15, 15, 150, 150);
    }
    while (display.nextPage());
    llog_i("drawGreyPixmap(img_tst150px2bpp, 2");
    delay(7000);


    render4GJpegFile("/300.jpg", &display, 10, 10, 115, 115);

    delay(12000);

    render4GJpegFile("/300.jpg", &display, 10, 10, 115, 115);

    delay(12000);

    render4GJpegFile("/panda.jpg", &display, 10, 10, 115, 115); //120, 160);
*/
    llog_i("DRAW completed");
}


void epaper_testt(int16_t x, int16_t y, int16_t w, int16_t h)
{
    llog_e("test x:%d, y:%d %dx%d", x, y, w, h);
    display.setFullWindow();
    display.firstPage();
    display.fillScreen(GxEPD_WHITE);
    display.fillRect(x, y, w, h, GxEPD_BLACK);
    display.nextPage();
    display.powerOff();
    delay(7000);
}

void logo_screen(String message)
{
    llog_i("logo_screen");

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

    llog_i("logo_screen end");
}

void main_interface()
{
    llog_i("main_interface");

    refresh_date_time_now();

    display.setFullWindow();
    display.firstPage();

    do
    {
        
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        //display.drawFastHLine(0, 124, 264, GxEPD_BLACK);
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

        display.drawRoundRect (3, 18, 140, 140, 5, GxEPD_BLACK);

/*
        for (int idx=1; idx<=3; idx++)
        {
            display.drawCircle(100 + 18 * idx * 2.5f, 115, 18, GxEPD_BLACK);
        }
*/
 
/*
Week day
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(time_x, time_y + 20);
        display.print(dtInfo.weekDay);
*/

        //display.drawGrayscaleBitmap();
        //display.drawImage(img_90bw_data, 0, 15, 90, 90, false, true, true);


        //display.drawBitmap(5, 20, epd_bitmap_90bw, 90, 90, GxEPD_BLACK);

    //display.drawGrayscaleBitmap(0, 18, Bi115bw, 115, 115);

    epaper_draw_station_title(station_title_x, station_title_y, "Radio:");
    epaper_draw_stream_title(stream_title_x, stream_title_y, "Streamm:");

    //display.epd2.drawImage_4G(Bi115bw, 8, 0, 0, 115, 115, false, false, true);
    //delay(5000);



        //display.fillRect(0, 18, 20, 20, GxEPD_BLACK);
        //display.fillRect(0, 18 + 25, 20, 20, GxEPD_DARKGREY);
        //display.fillRect(0, 18 + 55, 20, 20, GxEPD_LIGHTGREY);

        //set_epaper_wifi_signal(245, 10, -60, true);

        //display.drawFastVLine(108, 0, 85, GxEPD_BLACK);
        //subrutine_battery(0);

    } while (display.nextPage());

    display.powerOff();

    //delay(3000);

    //display.setPartialWindow(0, 20, 115, 115);
    //render4GJpegFile("/300.jpg", &display, 5, 20, 115, 115);

    //display.firstPage();

    //llog_e("EPD: epd2.hasPartialUpdate %d, display.epd2.hasFastPartialUpdate %d", display.epd2.hasPartialUpdate, display.epd2.hasFastPartialUpdate);

    llog_i("main_interface end");


/*
    do{
        display.drawBitmap(5, 20, epd_bitmap_90bw, 90, 90, GxEPD_DARKGREY);
    } while (display.nextPage());

    display.powerOff();

    delay(3000);

    do{
        display.drawBitmap(5, 20, epd_bitmap_90bw, 90, 90, GxEPD_LIGHTGREY);
    } while (display.nextPage());

    display.powerOff();

    delay(3000);
    */

}

/*
void helloWorld()
{
  //Serial.println("helloWorld");
  const char HelloWorld[] = "Hello World!";

  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(HelloWorld);
  }
  while (display.nextPage());
  //Serial.println("helloWorld done");
}

void showPartialUpdate()
{
  // some useful background
  helloWorld();
  // use asymmetric values for test
  uint16_t box_x = 10;
  uint16_t box_y = 15;
  uint16_t box_w = 70;
  uint16_t box_h = 20;
  uint16_t cursor_y = box_y + box_h - 6;
  float value = 13.95;
  uint16_t incr = display.epd2.hasFastPartialUpdate ? 3 : 3;
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  // show where the update box is
  for (uint16_t r = 0; r < 4; r++)
  {
    display.setRotation(r);
    display.setPartialWindow(box_x, box_y, box_w, box_h);
    display.firstPage();
    do
    {
      display.fillRect(box_x, box_y, box_w, box_h, GxEPD_BLACK);
      //display.fillScreen(GxEPD_BLACK);
    }
    while (display.nextPage());
    delay(2000);
    display.firstPage();
    do
    {
      display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }
    while (display.nextPage());
    delay(1000);
  }
  //return;
  // show updates in the update box
  for (uint16_t r = 0; r < 4; r++)
  {
    display.setRotation(r);
    display.setPartialWindow(box_x, box_y, box_w, box_h);
    for (uint16_t i = 1; i <= 10; i += incr)
    {
      display.firstPage();
      do
      {
        display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
        display.setCursor(box_x, cursor_y);
        display.print(value * i, 2);
      }
      while (display.nextPage());
      delay(500);
    }
    delay(1000);
    display.firstPage();
    do
    {
      display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }
    while (display.nextPage());
    delay(1000);
  }
}

*/
