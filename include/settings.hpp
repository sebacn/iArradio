#ifndef _settings_h
#define _settings_h

#define HOLD_TIMEOUT 2000
#define DEBOUNCETIME 60

#define WIFI_AP_NAME "iArradio"
#define WIFI_AP_PASS "iArradio123"

String dbgPrintln(String _str);
String infoPrintln(String _str);
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
        //dbgPrintln("City (" + String(sizeof(City)) + "):");
        //dbgPrintln("Settings (" + String(sizeof(this)) + "):");

        dbgPrintln("Settings: City: " + City 
            + ", (lat, lon): (" + Latitude + ", " + Longitude + ")"
            + ", Units: " + Units);

        dbgPrintln("Settings: OwmApikey: " + OwmApikey 
            + ", TimezBBKey: " + TimezBBKey 
            + ", PositionStackKey: " + PositionStackKey 
            + ", ConfigOk: " + String(ConfigOk)); 
    }
    
} ;

#endif