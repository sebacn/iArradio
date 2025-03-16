#include "wifi_hardware.hpp"
#include "controls.hpp"
#include "settings.hpp"
#include "api_request.hpp"
#include "locallog.hpp"

WiFiManager wifiManager;
WiFiManagerParameter *pparmCity;
WiFiManagerParameter *pparmOwmApikey;
WiFiManagerParameter *pparmPositionStackKey;   
WiFiManagerParameter *pparmTimezdbKey;
WiFiManagerParameter *pparmUnits;

bool configSaved;
extern Settings settings;
//extern GxEPD2_4G_4G<GxEPD2_270, GxEPD2_270::HEIGHT> display;
boolean UpdateLocalTime();

uint8_t StartWiFi() {
    llog_d("Connecting to: %s", settings.WiFiSSID);

    if (WiFi.status() == WL_CONNECTED) 
    {
        llog_i("WiFi already connected at: %s", WiFi.localIP().toString());
        return WL_CONNECTED;
    }

    IPAddress dns(8, 8, 8, 8); // Use Google DNS
    WiFi.disconnect();
    WiFi.mode(WIFI_STA); // switch off AP
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.begin(settings.WiFiSSID.c_str(), settings.WiFiPass.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        llog_d("STA: Failed!");
        WiFi.disconnect(false);
        delay(500);
        WiFi.begin(settings.WiFiSSID.c_str(), settings.WiFiPass.c_str());
    }
    if (WiFi.status() == WL_CONNECTED) {
        //wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
        llog_i("WiFi connected at: %s", WiFi.localIP().toString());
    }
    else llog_i("WiFi connection *** FAILED ***");
    return WiFi.status();
}

void StopWiFi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    llog_d("WiFi switched Off");
}

//callback notifying us of the need to save config
void saveConfigCallback () {
    llog_d("Should save config");

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
    llog_d("WiFi Init");
    read_config_from_memory();

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

    logo_screen("Connecting to WiFi: " + settings.WiFiSSID);
    llog_i("Connecting to WiFi: %s", settings.WiFiSSID.c_str());

    if (settings.ConfigOk)
    {
        if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASS))
        {
            llog_d("Failed to connect and hit timeout");
            logo_screen("Failed connect to WiFi, start web-portal (60 sec timeout)..\n" + CONFIGURE_WIFI + "\nSSID:" + WIFI_AP_NAME + " PASS:" + WIFI_AP_PASS);
            wifiManager.startConfigPortal(WIFI_AP_NAME, WIFI_AP_PASS);
            //power_off();
        }
    }
    else
    {
        llog_d("Settings is not configured/validated");
        logo_screen("Settings is not configured/validated.\nTo configure settings connect to " + String(WIFI_AP_NAME) + " PASS:" + WIFI_AP_PASS);
        wifiManager.startConfigPortal(WIFI_AP_NAME, WIFI_AP_PASS);
    }
    
    if (WiFi.status() != WL_CONNECTED)
    {
        delay(3000);
    }

    llog_i("WiFi status: %d", WiFi.status());
}

void configModeCallback(WiFiManager *myWiFiManager)
{
    llog_d("configModeCallback");
    logo_screen(CONFIGURE_WIFI + "\nSSID:" + WIFI_AP_NAME + " PASS:" + WIFI_AP_PASS);
}

bool validate_settings()
{
    bool ret = true;
    String keyErrMsg;

    llog_d("Validate settings");

    if (StartWiFi() == WL_CONNECTED) {

        llog_d("Wifi connected, validate settings...");

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

        llog_d("Validate key, missing keys: %s", (keyErrMsg == ""? "No" : keyErrMsg));

        if (keyErrMsg != "")
        {
          logo_screen("Parameter(s) is/are not configured:\n" + keyErrMsg + "\n" + "Restarting in 10 sec");  
          ret = false;
        }
        else
        {
            llog_d("Validate: get locations by names");

            if (http_request_location(&settings))
            {
                if (atof(settings.Latitude.c_str()) == 0 
                 && atof(settings.Longitude.c_str()) == 0 )
                {
                    keyErrMsg += llog_d("Validate: location for %s fetch FAILED (lat:0,lon:0)", settings.City);
                    ret = false;
                }
                else
                {
                    llog_d("Validate: location for %s fetched Ok", settings.City);
                }
            }
            else
            {
                keyErrMsg += llog_d("Validate: location %s fetch FAILED", settings.City);
                ret = false;
            }  
        }

        if (ret) //loction ok
        {
            bool is_time_fetched = http_request_datetime(&settings);
            bool is_weather_fetched = http_request_weather(&settings);

            if (!is_time_fetched)
            {
                keyErrMsg += llog_d("Time fetch error\nTIMEZDB_KEY not valid");
                ret = false;
            }

            if (!is_weather_fetched)
            {
                keyErrMsg += llog_d("Weather fetch error\nOPENWEATHER_KEY not valid");
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
        llog_d("Wifi connection failed, reboot to config...");
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

                llog_d("wifi_rutine configSaved");
            
            if (!validate_settings())
            {
                llog_d("Settings validation failed");
                logo_screen("Settings validation failed..\nTo configure settings connect to " + settings.WiFiSSID + " PASS:" + WIFI_AP_PASS);
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