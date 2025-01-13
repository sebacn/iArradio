#include "wifi_hardware.hpp"
#include "controls.hpp"
#include "settings.hpp"
#include "api_request.hpp"

WiFiManager wifiManager;
WiFiManagerParameter *pparmCity;
WiFiManagerParameter *pparmOwmApikey;
WiFiManagerParameter *pparmPositionStackKey;   
WiFiManagerParameter *pparmTimezdbKey;
WiFiManagerParameter *pparmUnits;

bool configSaved;
extern Settings settings;
boolean UpdateLocalTime();

uint8_t StartWiFi() {
    dbgPrintln("Connecting to: " + settings.WiFiSSID);

    if (WiFi.status() == WL_CONNECTED) 
    {
        infoPrintln("WiFi connected at: " + WiFi.localIP().toString());
        return WL_CONNECTED;
    }

    IPAddress dns(8, 8, 8, 8); // Use Google DNS
    WiFi.disconnect();
    WiFi.mode(WIFI_STA); // switch off AP
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.begin(settings.WiFiSSID.c_str(), settings.WiFiPass.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        dbgPrintln("STA: Failed!");
        WiFi.disconnect(false);
        delay(500);
        WiFi.begin(settings.WiFiSSID.c_str(), settings.WiFiPass.c_str());
    }
    if (WiFi.status() == WL_CONNECTED) {
        //wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
        infoPrintln("WiFi connected at: " + WiFi.localIP().toString());
    }
    else infoPrintln("WiFi connection *** FAILED ***");
    return WiFi.status();
}

void StopWiFi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    dbgPrintln("WiFi switched Off");
}

//callback notifying us of the need to save config
void saveConfigCallback () {
    dbgPrintln("Should save config");

    settings.WiFiPass = wifiManager.getWiFiPass();
    settings.WiFiSSID = wifiManager.getWiFiSSID();
    settings.City = String(pparmCity->getValue());
    settings.OwmApikey = String(pparmOwmApikey->getValue());
    settings.PositionStackKey = String(pparmPositionStackKey->getValue());
    settings.TimezBBKey = String(pparmTimezdbKey->getValue());
    settings.ConfigOk = false;

    save_config_to_memory();

    configSaved = true;
}

void init_wifi()
{    
    dbgPrintln("Init wifi");
    print_pt();
    read_config_from_memory();
    wakeup_reason();

    wifiManager.setDebugOutput(true);
    wifiManager.setAPCallback(configModeCallback);
    //wifiManager.setTimeout(60*1);
    wifiManager.setConfigPortalTimeout(60*5);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setConfigPortalBlocking(false);

    pparmCity = new WiFiManagerParameter("parmCity", "City", settings.City.c_str(), 15);
    pparmOwmApikey = new WiFiManagerParameter("parmOwmApikey", "Openweathermap api key", settings.OwmApikey.c_str(), 40);
    pparmPositionStackKey = new WiFiManagerParameter("parmPositionStackKey", "PositionStack api key", settings.PositionStackKey.c_str(), 40);    
    pparmTimezdbKey = new WiFiManagerParameter("parmTimezdbKey", "Timezdb key", settings.TimezBBKey.c_str(), 20);
    pparmUnits = new WiFiManagerParameter("parmUnits", "Units (M/I)", settings.Units.c_str(), 3);

    wifiManager.addParameter(pparmCity);
    wifiManager.addParameter(pparmOwmApikey);
    wifiManager.addParameter(pparmPositionStackKey);
    wifiManager.addParameter(pparmTimezdbKey);
    wifiManager.addParameter(pparmUnits);

    logo_screen("Connecting to WiFi: " + String(WIFI_AP_NAME));

    if (settings.ConfigOk)
    {
        if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASS))
        {
            dbgPrintln("Failed to connect and hit timeout");
            logo_screen("Failed connect to WiFi, start web-portal (60 sec timeout)..\n" + CONFIGURE_WIFI + "\nSSID:" + WIFI_AP_NAME + " PASS:" + WIFI_AP_PASS);
            wifiManager.startConfigPortal(WIFI_AP_NAME, WIFI_AP_PASS);
            //power_off();
        }
    }
    else
    {
        dbgPrintln("Settings is not configured/validated");
        logo_screen("Settings is not configured/validated.\nTo configure settings connect to " + String(WIFI_AP_NAME) + " PASS:" + WIFI_AP_PASS);
        wifiManager.startConfigPortal(WIFI_AP_NAME, WIFI_AP_PASS);
    }
    
    if (WiFi.status() != WL_CONNECTED)
    {
        delay(3000);
    }
}

void configModeCallback(WiFiManager *myWiFiManager)
{
    dbgPrintln("configModeCallback");
    logo_screen(CONFIGURE_WIFI + "\nSSID:" + WIFI_AP_NAME + " PASS:" + WIFI_AP_PASS);
}

bool validate_settings()
{
    bool ret = true;
    String keyErrMsg;

    dbgPrintln("Validate settings");

    if (StartWiFi() == WL_CONNECTED) {

        dbgPrintln("Wifi connected, validate settings...");

        if (settings.City == "")
        {
          keyErrMsg = "City,";
        }

        if (settings.OwmApikey == "")
        {
          keyErrMsg += "OwmApikey,";
        } 

        if (settings.PositionStackKey == "")
        {
          keyErrMsg += "PositionStackKey,";
        } 

        if (settings.TimezBBKey == "")
        {
          keyErrMsg += "TimezBBKey,";
        }

        dbgPrintln("Validate key, missing keys: " + (keyErrMsg == ""? "No" : keyErrMsg));

        if (keyErrMsg != "")
        {
          logo_screen("Parameter(s) is/are not configured:\n" + keyErrMsg + "\n" + "Restarting in 10 sec");  
          ret = false;
        }
        else
        {
            dbgPrintln("Validate: get locations by names");

            if (http_request_location(&settings))
            {
                if (atof(settings.Latitude.c_str()) == 0 
                 && atof(settings.Longitude.c_str()) == 0 )
                {
                    keyErrMsg += dbgPrintln("Validate: location for " + settings.City + " fetch FAILED (lat:0,lon:0)");
                    ret = false;
                }
                else
                {
                    dbgPrintln("Validate: location for " + settings.City + " fetched Ok");
                }
            }
            else
            {
                keyErrMsg += dbgPrintln("Validate: location " + settings.City + " fetch FAILED");
                ret = false;
            }  
        }

        if (ret) //loction ok
        {
            bool is_time_fetched = http_request_datetime(&settings);
            bool is_weather_fetched = http_request_weather(&settings);

            if (!is_time_fetched)
            {
                keyErrMsg += dbgPrintln("Time fetch error\nTIMEZDB_KEY not valid");
                ret = false;
            }

            if (!is_weather_fetched)
            {
                keyErrMsg += dbgPrintln("Weather fetch error\nOPENWEATHER_KEY not valid");
                ret = false;
            }
        } 

        if (!ret)
        {
            logo_screen("Validation failed:\n" + keyErrMsg);
        } 
    }
    else
    {
        dbgPrintln("Wifi connection failed, reboot to config...");
        logo_screen("Wifi connection failed\nReboot to config...");
        ret = false;
    }

    settings.ConfigOk = ret;
    save_config_to_memory();

    if (ret)
    { 
        UpdateLocalTime();
    }
    else
    {
        delay(7000); //display
    }

    return ret;
}

void wifi_rutine()
{
    if (wifiManager.getConfigPortalActive()) // getWebPortalActive())
    {
        wifiManager.process();
    }
    else 
    {
        if (configSaved) //validate settings
        {
            configSaved = false;

            dbgPrintln("wifi_rutine configSaved");
            
            if (!validate_settings())
            {
                dbgPrintln("Settings validation failed");
                logo_screen("Settings validation failed..\nTo configure settings connect to " + String(WIFI_AP_NAME) + " PASS:" + WIFI_AP_PASS);
                wifiManager.startConfigPortal(WIFI_AP_NAME, WIFI_AP_PASS);
            }
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            logo_screen("WiFi not connected, switch off..");
            delay(3000);
            power_off();
        }
        
    }
}