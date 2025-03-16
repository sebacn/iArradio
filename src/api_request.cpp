#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson
#include <HTTPClient.h>         // In-built
#include <WiFi.h>               // In-built
#include <SPI.h>                // In-built
#include "api_request.hpp"
#include "locallog.hpp"

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

    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = datetime_request.response.dt;
    settimeofday(&tv, NULL);
    setenv("TZ", "UTC0", 1); // https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
    tzset();

    llog_d("DtaTime updated (update_datetime)");
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
    llog_d("Geocoding...");
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
    llog_d("Deserializing json, size: x bytes...");
    JsonDocument doc;//(size);
    DeserializationError error;
    
    if (is_embeded) {
        String stream_as_string = resp_stream.readString();
        int begin = stream_as_string.indexOf('{');
        int end = stream_as_string.lastIndexOf('}');
        llog_d("Embeded json algorithm obtained document...");
        String trimmed_json = stream_as_string.substring(begin, end+1);
        llog_d("JSON: %s", trimmed_json.c_str());
        error = deserializeJson(doc, trimmed_json);
    } else {
        error = deserializeJson(doc, resp_stream);
    }
    if (error) {
        llog_d("deserialization error: %s\n", error.c_str());
    } else {
        llog_d("deserialized.\n");
    }
    return doc;
}

bool http_request_data(Request request, unsigned int retry) 
{    
    bool ret_val = false;

    while (!ret_val && retry--) {
        ret_val = true;
        client.stop();
        HTTPClient http;
        llog_i("HTTP connecting to %s%s [retry left: %d]", request.server.c_str(), request.path.c_str(), retry);
        client.setCACert(request.ROOT_CA);
        http.begin(client, request.server, 443, request.path);
        int http_code = http.GET();
        
        if(http_code == HTTP_CODE_OK) {
            llog_i("HTTP connection OK");
            if (!request.handler(http.getStream(), request)) {
                ret_val = false;
            }
        } else {
            llog_i("HTTP connection failed %d, error: %s", http_code, http.errorToString(http_code));
            ret_val = false;
        }
        client.stop();
        http.end();
    }
    return ret_val;
}


bool http_request_datetime(Settings *settings)
{
    llog_d("HTTP request datetime");
    datetime_request.api_key = settings->TimezBBKey;            
    datetime_request.handler = datetime_handler;
    datetime_request.make_path(settings->Latitude, settings->Longitude);

    return http_request_data(datetime_request);        
}

bool http_request_location(Settings *settings)
{
    llog_d("HTTP request location");
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
    llog_d("HTTP request weather");
    weather_request.api_key = settings->OwmApikey;
    weather_request.handler = weather_handler;
    weather_request.make_path(settings->Latitude, settings->Longitude, settings->Units);

    return http_request_data(weather_request);
}