#include <Arduino.h>

//#include <time.h>               // In-built
#include <RTClib.h>
#include <qrcode.h>
#include <Preferences.h>
#include <rom/rtc.h>
#include "api_request.hpp"
#include "settings.hpp"

#define MEMORY_ID "mem"

Settings settings;
Preferences preferences;

QRCode qrcode;

String infoPrintln(String _str) {
    Serial.printf("%.03f ",millis()/1000.0f); Serial.println("[I] " + _str);
    return _str + '\n';
}

String dbgPrintln(String _str) {
    #if defined(PROJ_DEBUG_ENABLE)
    if (true) //settings.DbgLogEnable)
    {
        Serial.printf("%.03f ",millis()/1000.0f); Serial.println("[D] " + _str);
    }    
    #endif
    return _str + '\n';
}

void print_pt()
{
  printf("\n\nESP32 Partition table:\n\n");

  printf("| Type | Sub |  Offset  |   Size   | Size (b) |       Label      |\n");
  printf("| ---- | --- | -------- | -------- | -------- | ---------------- |\n");
  
  esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (pi != NULL) {
    do {
      const esp_partition_t* p = esp_partition_get(pi);
      printf("|  %02x  | %02x  | 0x%06X | 0x%06X | %8d | %-16s |\r\n", 
        p->type, p->subtype, p->address, p->size, p->size, p->label);
    } while (pi = (esp_partition_next(pi)));
  }

  uint32_t program_size = ESP.getSketchSize();
  uint32_t free_size = ESP.getFlashChipSize();
  uint32_t psram_size = ESP.getPsramSize();
  uint32_t free_sketch_space = ESP.getFreeSketchSpace();

  Serial.println("");
  infoPrintln("Build date time: " + String(__DATE__) + " " + __TIME__);
  infoPrintln("Sketch size: " + String(program_size));
  infoPrintln("Free sketch space: " + String(free_sketch_space));
  infoPrintln("Flash chip size: " + String(free_size));
  infoPrintln("Psram size: " + String(psram_size));
  infoPrintln("Stack size: " + String(CONFIG_ARDUINO_LOOP_STACK_SIZE));
  infoPrintln("uxTaskGetStackHighWaterMark: " + String(uxTaskGetStackHighWaterMark(NULL)) + "\n\n"); 
}

String print_reset_reason(RESET_REASON reason) {
    String ret = "";
    switch ( reason) {
        case 1 : ret = "POWERON_RESET"; break;
        case 3 : ret = "SW_RESET"; break;
        case 4 : ret = "OWDT_RESET"; break;
        case 5 : ret = "DEEPSLEEP_RESET"; break;
        case 6 : ret = "SDIO_RESET"; break; 
        case 7 : ret = "TG0WDT_SYS_RESET"; break;
        case 8 : ret = "TG1WDT_SYS_RESET"; break;
        case 9 : ret = "RTCWDT_SYS_RESET"; break;
        case 10 : ret = "INTRUSION_RESET"; break;
        case 11 : ret = "TGWDT_CPU_RESET"; break;
        case 12 : ret = "SW_CPU_RESET"; break;
        case 13 : ret = "RTCWDT_CPU_RESET"; break;
        case 14 : ret = "EXT_CPU_RESET"; break;
        case 15 : ret = "RTCWDT_BROWN_OUT_RESET"; break;
        case 16 : ret = "RTCWDT_RTC_RESET"; break;
        default : ret = "UNKNOWN";
    }
    return ret;
}


void wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    dbgPrintln("CPU0 reset reason: " + print_reset_reason(rtc_get_reset_reason(0)));
    dbgPrintln("CPU1 reset reason: " + print_reset_reason(rtc_get_reset_reason(1)) + "\n");
    
    switch(wakeup_reason){
        //dbgPrintln("Location variable: " + String(curr_loc));
        
        case ESP_SLEEP_WAKEUP_EXT0 : dbgPrintln("Wakeup by ext signal RTC_IO -> GPIO39"); break;      
        case ESP_SLEEP_WAKEUP_EXT1 : dbgPrintln("Wakeup by ext signal RTC_CNTL -> GPIO34"); break;            
        case ESP_SLEEP_WAKEUP_TIMER : dbgPrintln("Wakeup by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : dbgPrintln("Wakeup by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : dbgPrintln("Wakeup by ULP program"); break;
        default : dbgPrintln("Wakeup not caused by deep sleep: " + String(wakeup_reason)); 
            if (rtc_get_reset_reason(0) == POWERON_RESET && rtc_get_reset_reason(1) == EXT_CPU_RESET)
            {
                //get_mode();
                //set_mode_and_reboot(CONFIG_MODE);
            }
        break;
    }
}

void read_config_from_memory() {
    dbgPrintln("Read config from memory...");

    preferences.begin(MEMORY_ID, true);  // first param true means 'read only'

    settings.WiFiSSID = preferences.getString("WiFiSSID","");
    settings.WiFiPass = preferences.getString("WiFiPass","");
    settings.OwmApikey = preferences.getString("OwmApikey","");     
    settings.City = preferences.getString("City","");                         
    settings.Latitude = preferences.getString("Latitude","");                        
    settings.Longitude = preferences.getString("Longitude","");  
    settings.TimezBBKey = preferences.getString("TimezBBKey","");
    settings.PositionStackKey = preferences.getString("PosStackKey","");
    settings.Units = preferences.getString("Units", "M");
    settings.ConfigOk = preferences.getBool("ConfigOk", false);
    
    preferences.end();

    settings.print();
}

void save_config_to_memory() {
    dbgPrintln("Save config to memory.");
    
    preferences.begin(MEMORY_ID, false);  // first param false means 'read/write'

    preferences.putString("WiFiSSID", settings.WiFiSSID);
    preferences.putString("WiFiPass", settings.WiFiPass);
    preferences.putString("OwmApikey", settings.OwmApikey);     
    preferences.putString("City", settings.City);                         
    preferences.putString("Latitude", settings.Latitude);                        
    preferences.putString("Longitude", settings.Longitude);  
    preferences.putString("TimezBBKey", settings.TimezBBKey);
    preferences.putString("PosStackKey", settings.PositionStackKey);
    preferences.putString("Units", settings.Units); 
    preferences.putBool("ConfigOk", settings.ConfigOk);

    preferences.end();

    settings.print();  
}