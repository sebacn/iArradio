#ifndef _api_request_h
#define _api_request_h

#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include "fmt.hpp"
//#include "config.h"
#include "ca_cert.hpp"
#include <HTTPClient.h> 
#include "settings.hpp"
#include "i18n.hpp"
#include "locallog.hpp"

struct Request;

bool http_request_datetime(Settings *settings);
bool http_request_location(Settings *settings);
bool http_request_weather(Settings *settings);

extern struct TimeZoneDbRequest datetime_request;

enum class ApiCall : byte
{
    OneCall  = 0,
    Weather  = 1,
    Forecast = 2,
    AQI      = 3
};

constexpr char const *api_names[] = {"onecall", "weather", "forecast", "air_pollution"};

typedef bool (*ResponseHandler) (WiFiClient& resp_stream, Request request);

struct Request {
    String server = "";
    String api_key = "";
    String path = "";
    ResponseHandler handler;
    char const *ROOT_CA = nullptr;    

    void make_path() {}
    
    String get_server_path() {
        return this->server + this->path;
    }

    bool UseHTTPS()
    {
        return (ROOT_CA == nullptr);
    }
} ;


struct TimeZoneDbResponse {
    time_t dt;
    int gmt_offset;
    int dst;
    String formatted;

    void print() {
        struct tm* ti;  // timeinfo
        ti = localtime(&dt); 
        char buffer[40];       
        //Serial.printf(
        sprintf(buffer,
            "Date and time:  %d:%d %d, %d-%d-%d ", 
            ti->tm_hour, ti->tm_min, ti->tm_wday, 
            ti->tm_mday, ti->tm_mon+1, 1900+ti->tm_year
        );
        llog_d("TimeZoneDbResponse: %s", buffer);
    }
} ;


struct TimeZoneDbRequest: Request {

    explicit TimeZoneDbRequest(): Request() {
        this->server = "api.timezonedb.com";
        this->ROOT_CA = ROOT_CA_TIMEZONEDB;
    }

    explicit TimeZoneDbRequest(String server, String api_key) {
        this->server = server;
        this->api_key = api_key;
    }
    
    void make_path(String _lat, String _lon) {
        this->path = "/v2.1/get-time-zone?key=" + api_key + "&format=json&by=position&lat=" + _lat + "&lng=" + _lon;
    }

    explicit TimeZoneDbRequest(const Request& request): Request(request) { }
    
    TimeZoneDbResponse response;
} ;


struct AirQualityResponse {
    int pm25;
    
    void print() {
        llog_d("Air quality (PM2.5): %d\n",pm25);
    }
} ;


struct AirQualityRequest: Request {
    
    explicit AirQualityRequest(): Request() {
        this->server = "api.waqi.info";
        this->ROOT_CA = ROOT_CA_WAQI;
    } 

    explicit AirQualityRequest(String server, String api_key) {
        this->server = server;
        this->api_key = api_key;
    }
    
    explicit AirQualityRequest(const Request& request): Request(request) { }

    void make_path(String _lat, String _lon) {
        this->path = "/feed/geo:" + _lat + ";" + _lon + "/?token=" + api_key;
    }
    
    AirQualityResponse response;
} ;


struct GeocodingNominatimResponse {
    float lat = 0.0f;
    float lon = 0.0f;
    String label = "";
    
    void print() {
        llog_d("City: %s (lat %f, lon %f)\n", label, lat, lon);
    }
} ;


struct GeocodingNominatimRequest: Request {
    String name = "";

    explicit GeocodingNominatimRequest(): Request() {
        this->server = "api.positionstack.com";
        this->ROOT_CA = ROOT_CA_POSITIONSTACK;
    }

    explicit GeocodingNominatimRequest(String server, String name) {
        this->server = server;
        this->name = name;
        make_path();
    }
    
    explicit GeocodingNominatimRequest(const Request& request): Request(request) { }

    void make_path() {
        this->path = "/v1/forward?access_key=" + api_key + "&query=" + name;
    }
    
    GeocodingNominatimResponse response;
} ;


struct WeatherResponseHourly {  // current and hourly
    int date_ts;
    int sunr_ts; // sunrise
    int suns_ts; // sunset
    int temp;  // round from float
    int feel_t;  // round from float
    int max_t; // [daily] round from float
    int min_t; // [daily] round from float
    int pressure; 
    int clouds;
    int wind_bft; // round from float to bft int
    int wind_deg; // round from float
    String icon;
    String descr;
    float snow;
    float rain;
    int pop; // [hourly] probability of percipitation hourly round to int percent
    
    void print() {
        char buffer[150];
        Serial.println("Weather currently: " + descr);        
        // 15 * 8 char strings
        sprintf(
            buffer, 
            "%8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s", 
            "date_ts", "sunr_ts", "suns_ts", "temp", "feel_t", 
            "max_t", "min_t", "pressure", "clouds", "wind_bft", 
            "wind_deg", "icon", "snow", "rain", "pop"
        );
        llog_d("WeatherResponseHourly1: %s", String(buffer));
        sprintf(
            buffer, 
            "%8s %8s %8s %8d %8d %8d %8d %8d %8d %8d %8d %8s %8.1f %8.1f %8d",
            ts2HM(date_ts), ts2HM(sunr_ts), ts2HM(suns_ts), 
            temp, feel_t, max_t, min_t,
            pressure, clouds, wind_bft, wind_deg,
            icon, snow, rain, pop
        );
        llog_d("WeatherResponseHourly2: %s", String(buffer));
    }
} ;


struct WeatherResponseDaily {
    int date_ts;
    int max_t;
    int min_t;
    int wind_bft;
    int wind_deg;
    int pop;
    float snow;
    float rain;

    void print() {
        char buffer[100];
        llog_d("Forecast: %s", ts2dm(date_ts));
        sprintf(
            buffer, 
            "%8s %8s %8s %8s %8s %8s %8s",
            "max_t", "min_t", "wind_bft", 
            "wind_deg", "pop", "snow", "rain"
        );
        llog_d("WeatherResponseDaily: %s", String(buffer));
        //Serial.printf("%15s", "");
        sprintf(
            buffer, 
            "%8d %8d %8d %8d %8d %8s %8s",
            max_t, min_t, wind_bft, wind_deg, pop,
            snow? "yes" : "no", rain? "yes" : "no"
        );
        llog_d("WeatherResponseDaily: %s", String(buffer));
    }

} ;


struct WeatherResponseRainHourly {
    int date_ts;
    int pop;
    float feel_t;
    float snow;
    float rain;
    String icon;

    void print() {
        char buffer[60];
        llog_d("Rain: %s", ts2date(date_ts));
        sprintf(
            buffer, 
            "%8s %8s %8s %8s %8s",
            "pop", "snow", "rain", "feel", "icon"
        );
        llog_d("WeatherResponseRainHourly: %s", String(buffer));
        //Serial.printf("%25s", "");
        sprintf(
            buffer, 
            "%8d %8.1f %8.1f %8.1f %8s",
            pop, snow, rain, feel_t, icon
        );
        llog_d("WeatherResponseRainHourly: %s", String(buffer));
    }
} ;


struct WeatherRequest: Request {
    ApiCall apiCall;

    explicit WeatherRequest(): Request() {
        this->server = "api.openweathermap.org";
        this->ROOT_CA = ROOT_CA_OWM;
    }
    
    explicit WeatherRequest(String server, String api_key) {
        this->server = server;
        this->api_key = api_key;
    }
    
    explicit WeatherRequest(const Request& request): Request(request) { }

    void make_path(String _lat, String _lon, String _units) {
        
        this->path = "/data/2.5/" + String(api_names[(byte)apiCall]) + "?lat=" + _lat + "&lon=" + _lon + "&appid=" + api_key + "&lang=" + LANGS[LANG]; 

        if (apiCall != ApiCall::AQI){
            this->path += "&mode=json&units=";
            this->path += _units == "M"? "metric" : "imperial" ;
        }

        if (apiCall == ApiCall::OneCall){
            this->path += "&exclude=minutely,hourly,alerts,daily";
        }
    }
    
    WeatherResponseHourly hourly[1];
    WeatherResponseDaily daily[2];
    WeatherResponseRainHourly rain[5];
} ;


#endif
