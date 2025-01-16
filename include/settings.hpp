#ifndef _settings_h
#define _settings_h

#include "locallog.hpp"

#define HOLD_TIMEOUT 2000
#define DEBOUNCETIME 60

#define WIFI_AP_NAME "iArradio"
#define WIFI_AP_PASS "iArradio123"

void print_pt();
void read_config_from_memory();
void save_config_to_memory();
void wakeup_reason();

struct Settings {

    bool ConfigOk;
    // Change to your WiFi credentials
    String WiFiSSID = "";
    String WiFiPass = "";

    // Use your own API key by signing up for a free developer account at https://openweathermap.org/
    String OwmApikey = "";   
    String TimezBBKey = "";
    String PositionStackKey = "";

    //Set your location according to OWM locations
    String City = "";                         
    String Latitude = "";                        
    String Longitude = "";
                                                         
    String Units = "M";        // Use 'M' for Metric or I for Imperial 

    void print() {
        llog_d("Settings: City: %s, lat %s, lon %s, Units %s", City.c_str(), Latitude.c_str(), Longitude.c_str(), Units.c_str());
        llog_d("Settings: OwmApikey: %s, TimezBBKey: %s, PositionStackKey: %s, ConfigOk: %d", OwmApikey.c_str(), TimezBBKey.c_str(), PositionStackKey.c_str(), ConfigOk);
    }
    
} ;

#endif