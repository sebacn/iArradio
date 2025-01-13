#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "wifi_hardware.hpp"
#include "eeprom_settings.hpp"
#include "epaper.hpp"
#include "icy_stream.hpp"
//#include "ntp_time.hpp"
#include "battery.hpp"
#include "user_input_buttons.hpp"
#include "tasks.hpp"
#include "hal.hpp"
#include "lang.hpp"
#include "api_request.hpp"

extern Settings settings;
String date_str = "1";
String time_str = "--:--:--";

boolean UpdateLocalTime() {
  //struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];

  if (datetime_request.response.dt == 0
   && !http_request_datetime(&settings))
  {
    return false;
  }

  struct tm timeinfo = *localtime(&datetime_request.response.dt);

  //See http://www.cplusplus.com/reference/ctime/strftime/
  Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");      // Displays: Saturday, June 24 2017 14:05:49
  if (settings.Units == "M") {
   
    sprintf(day_output, "%02u-%s-%04u", timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900); //weekday_D[timeinfo.tm_wday]
    
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '@ 14:05:49'   and change from 30 to 8 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    sprintf(time_output, "%s", update_time);
  }
  else
  {
    strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo); // Creates  'Sat May-31-2019'
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);        // Creates: '@ 02:05:49pm'
    sprintf(time_output, "%s", update_time);
  }
  date_str = day_output;
  time_str = time_output;
  return true;
}

void setup()
{
  Serial.begin(115200);
  init_display();
  init_wifi();
  //init_ntp();
  UpdateLocalTime();
  main_interface();
  eeprom_init();
  set_volume(eeprom_get_volume());
  set_station(eeprom_get_station());
  init_audio();
  configure_buttons();
  
  xTaskCreate(task_epaper_header, "TaskEpaperHeader", 2500, NULL, tskIDLE_PRIORITY, NULL);
  xTaskCreate(task_epaper_rssi, "TaskEpaperRSSI", 2500, NULL, tskIDLE_PRIORITY, NULL);
  //xTaskCreate(task_ntp, "TaskNTP", 1500, NULL, tskIDLE_PRIORITY, NULL);
  //xTaskCreate(task_weather, "TaskWEATHER", 35000, NULL, tskIDLE_PRIORITY, NULL);

  handle_home();
}

void loop()
{
  audio_rutine();
  buttons_rutine();
  wifi_rutine();
}


