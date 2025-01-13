#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson
#include <HTTPClient.h>         // In-built
#include <WiFi.h>               // In-built
#include <SPI.h>                // In-built
#include "api_request.hpp"

String dbgPrintln(String _str);
String infoPrintln(String _str);

WiFiClientSecure client;

struct GeocodingNominatimRequest location_request;
struct TimeZoneDbRequest datetime_request;
struct WeatherRequest weather_request;

bool http_request_data(Request request, unsigned int retry = 3);
JsonDocument deserialize(WiFiClient& resp_stream, bool is_embeded = false);

String openweather_icons[9] = {
    "01",   // 0 clear sky
    "02",   // 1 few clouds    
    "03",   // 2 scattered clouds
    "04",   // 3 broken clouds
    "09",   // 4 shower rain
    "10",   // 5 rain
    "11",   // 6 thunderstorm
    "13",   // 7 snow
    "50"    // 8 mist
};

void update_location(GeocodingNominatimResponse& location_resp, JsonObject& jobj) {
    location_resp.lat = jobj["data"][0]["latitude"].as<float>();
    location_resp.lon = jobj["data"][0]["longitude"].as<float>();
    location_resp.label = String(jobj["data"][0]["label"].as<String>()).substring(0, 25); //char*
}

void update_datetime(TimeZoneDbResponse& datetime_resp, JsonObject& jobj) {
    datetime_resp.dt = jobj["timestamp"].as<int>();
    datetime_resp.gmt_offset = jobj["gmtOffset"].as<int>();
    datetime_resp.dst = jobj["dst"].as<int>();
    datetime_resp.formatted = jobj["formatted"].as<String>();
}

bool location_handler(WiFiClient& resp_stream, Request request) {
    //const int json_size = 20 * 1024;
    JsonDocument doc = deserialize(resp_stream, true);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }
    location_request = GeocodingNominatimRequest(request);
    GeocodingNominatimResponse& location_resp = location_request.response;
    dbgPrintln("Geocoding...");
    update_location(location_resp, api_resp);
    location_resp.print();
    return true;
}

bool datetime_handler(WiFiClient& resp_stream, Request request) {
    //const int json_size = 10 * 1024;
    JsonDocument doc = deserialize(resp_stream, true);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }
    datetime_request = TimeZoneDbRequest(request);
    TimeZoneDbResponse& datetime_response = datetime_request.response;
    update_datetime(datetime_response, api_resp);
    datetime_response.print();
    return true;
}

bool weather_handler(WiFiClient& resp_stream, Request request) {

    JsonDocument doc = deserialize(resp_stream);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }

    return true;
}

JsonDocument deserialize(WiFiClient& resp_stream, bool is_embeded) {
    // https://arduinojson.org/v6/assistant/
    dbgPrintln("Deserializing json, size: x bytes...");
    JsonDocument doc;//(size);
    DeserializationError error;
    
    if (is_embeded) {
        String stream_as_string = resp_stream.readString();
        int begin = stream_as_string.indexOf('{');
        int end = stream_as_string.lastIndexOf('}');
        dbgPrintln("Embeded json algorithm obtained document...");
        String trimmed_json = stream_as_string.substring(begin, end+1);
        dbgPrintln(trimmed_json);
        error = deserializeJson(doc, trimmed_json);
    } else {
        error = deserializeJson(doc, resp_stream);
    }
    if (error) {
        dbgPrintln(F("deserialization error:"));
        dbgPrintln(error.c_str());
    } else {
        dbgPrintln("deserialized.");
    }
    dbgPrintln("");
    return doc;
}

bool http_request_data(Request request, unsigned int retry) 
{    
    bool ret_val = false;

    while (!ret_val && retry--) {
        ret_val = true;
        client.stop();
        HTTPClient http;
        infoPrintln("HTTP connecting to " + request.server + request.path + " [retry left: " + String(retry) + "]");
        client.setCACert(request.ROOT_CA);
        http.begin(client, request.server, 443, request.path);
        int http_code = http.GET();
        
        if(http_code == HTTP_CODE_OK) {
            infoPrintln("HTTP connection OK");
            if (!request.handler(http.getStream(), request)) {
                ret_val = false;
            }
        } else {
            infoPrintln("HTTP connection failed " + String(http_code) + ", error: " + http.errorToString(http_code));
            ret_val = false;
        }
        client.stop();
        http.end();
    }
    return ret_val;
}


bool http_request_datetime(Settings *settings)
{
    dbgPrintln("http_request_datetime");
    datetime_request.api_key = settings->TimezBBKey;            
    datetime_request.handler = datetime_handler;
    datetime_request.make_path(settings->Latitude, settings->Longitude);

    return http_request_data(datetime_request);        
}

bool http_request_location(Settings *settings)
{
    dbgPrintln("http_request_location");
    location_request.handler = location_handler;
    location_request.name = settings->City;
    location_request.api_key = settings->PositionStackKey;
    location_request.make_path();

    bool ret = http_request_data(location_request);
    if (ret)
    {
        settings->Latitude = location_request.response.lat;
        settings->Longitude = location_request.response.lon;
    }

    return ret;
}

bool http_request_weather(Settings *settings)
{
    dbgPrintln("http_request_weather");
    weather_request.api_key = settings->OwmApikey;
    weather_request.handler = weather_handler;
    weather_request.make_path(settings->Latitude, settings->Longitude, settings->Units);

    return http_request_data(weather_request);
}