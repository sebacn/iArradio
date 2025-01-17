#include <Arduino.h>
#include "icy_stream.hpp"
#include "stations.hpp"
#include "tasks.hpp"

Audio audio;
String stream_title;
String station_title;
uint8_t volume = 10;
uint16_t station_index = 0;
//char streamTitle[512] = {};
char* stationName_air = NULL;
uint8_t commercial_dur = 0;
char  commercial[25];
char  icyDescription[512] = {};
uint16_t icyBitRate = 0;

void init_audio()
{
    audio.setPinout(BCLK_PIN, LRC_PIN, DOUT_PIN);
    audio.setVolume(volume);
    change_station(0);
}

void audio_rutine()
{
    audio.loop();
}

void audio_stop()
{
    audio.stopSong();
}
/*
void audio_info(const char *info)
{
    Serial.print("info ");
    Serial.println(info);
}
*/
void change_station(int8_t direction)
{
    if (station_index == 0 && direction == -1)
    {
        station_index = TOTAL_STATIONS - 1;
    }
    else
    {
        station_index += direction;
    }

    if (station_index >= TOTAL_STATIONS)
    {
        station_index = 0;
    }
    audio.stopSong();

    if (WiFi.status() == WL_CONNECTED)
    {
        audio.connecttohost(stations[station_index].url.c_str());
        station_title = String(LISTENING + ": " + stations[station_index].name);        
    }
    else
    {
        station_title = String("No WiFi : " + stations[station_index].name); 
    }

    //xTaskCreate(task_stream_title, "TaskEpaperStation", 5000, NULL, 1, NULL);
    xTaskCreate(task_epaper_station_number, "TaskEpaperStationNumber", 5000, &station_index, 1, NULL);
    xTaskCreate(task_eeprom_station, "TaskEEPROMStation", 5000, &station_index, 1, NULL);   
}

void set_station(int8_t index)
{
    if (index < 0 || index >= TOTAL_STATIONS)
    {
        station_index = 0;
    }
    station_index = index;
}

/*
void audio_showstreamtitle(const char *info)
{
    station_text = String(info);
    // xTaskCreate(task_stream_title, "TaskEpaperStation", 5000, &station_text, 1, NULL);
    // THIS IS A BIT BUGGY
}
*/

void increase_volume(int8_t amount)
{
    volume += amount;
    if (volume > 21)
        volume = 21;
    if (volume < 0)
        volume = 0;
    audio.setVolume(volume);
    xTaskCreate(task_epaper_volume, "TaskEpaperVolume", 5000, &volume, 1, NULL);
    xTaskCreate(task_eeprom_volume, "TaskEEPROMVolume", 5000, &volume, 1, NULL);
}

void set_volume(int8_t value)
{
    if (value > 21)
        value = 21;
    if (value < 0)
        value = 0;
    volume = value;
    increase_volume(0);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
inline int32_t str2int(const char* str) {
    int32_t len = strlen(str);
    if(len > 0) {
        for(int32_t i = 0; i < len; i++) {
            if(!isdigit(str[i])) {
                log_e("NaN");
                return 0;
            }
        }
        return stoi(str);
    }
    return 0;
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
inline void x_ps_free(char** b){
    if(*b){free(*b); *b = NULL;}
}
inline void x_ps_free(unsigned char** b){
    if(*b){free(*b); *b = NULL;}
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline char* x_ps_malloc(uint16_t len) {
    char* ps_str = NULL;
    if(psramFound()){ps_str = (char*) ps_malloc(len);}
    else             {ps_str = (char*)    malloc(len);}
    if(!ps_str){log_e("oom");}
    return ps_str;
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline char* x_ps_calloc(uint16_t len, uint8_t size) {
    char* ps_str = NULL;
    if(psramFound()){ps_str = (char*) ps_calloc(len, size);}
    else             {ps_str = (char*)    calloc(len, size);}
    if(!ps_str){log_e("oom");}
    return ps_str;
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline char* x_ps_strdup(const char* str) {
    if(!str){log_e("str is NULL"); return NULL;}
    char* ps_str = NULL;
    if(psramFound()) { ps_str = (char*)ps_malloc(strlen(str) + 1); }
    else { ps_str = (char*)malloc(strlen(str) + 1); }
    if(!ps_str){log_e("oom"); return NULL;}
    strcpy(ps_str, str);
    return ps_str;
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline char* x_ps_strndup(const char* str, uint16_t n) { // with '\0' termination
    if(!str){log_e("str is NULL"); return NULL;}
    char* ps_str = NULL;
    if(psramFound()) { ps_str = (char*)ps_malloc(n + 1); }
    else { ps_str = (char*)malloc(n + 1); }
    if(!ps_str){log_e("oom"); return NULL;}
    strncpy(ps_str, str, n);
    ps_str[n] = '\0';
    return ps_str;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline bool startsWith(const char* base, const char* searchString) {
    if(base == NULL) {log_e("base = NULL"); return false;}                      // guard
    if(searchString == NULL) {log_e("searchString == NULL"); return false;}     // guard
    if(strlen(searchString) > strlen(base)) return false;
    char c;
    while((c = *searchString++) != '\0')
        if(c != *base++) return false;
    return true;
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline bool endsWith(const char* base, const char* searchString) {
    if(base == NULL) {log_e("base = NULL"); return false;}                      // guard
    if(searchString == NULL) {log_e("searchString == NULL"); return false;}     // guard
    int32_t slen = strlen(searchString);
    if(slen == 0) return false;
    const char* p = base + strlen(base);
    //  while(p > base && isspace(*p)) p--;  // rtrim
    p -= slen;
    if(p < base) return false;
    return (strncmp(p, searchString, slen) == 0);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// Events from audioI2S library
void audio_info(const char* info) {
    if(endsWith(  info, "failed!"))                {llog_w("AUDIO_info:  ", String(info).substring(0, 100).c_str()); return;}
    if(startsWith(info, "FLAC"))                   {llog_i("AUDIO_info:  ", String(info).substring(0, 100).c_str()); return;}
    if(endsWith(  info, "Stream lost"))            {llog_w("AUDIO_info:  ", String(info).substring(0, 100).c_str()); return;}
    if(startsWith(info, "authent"))                {llog_i("AUDIO_info:  ", String(info).substring(0, 100).c_str()); return;}
    if(startsWith(info, "StreamTitle="))           {return;}
    if(startsWith(info, "HTTP/") && info[9] > '3') {llog_e("AUDIO_info:  ", String(info).substring(0, 100).c_str()); return;}
    if(startsWith(info, "ERROR:"))                 {llog_e("AUDIO_info:  ", String(info).substring(0, 100).c_str()); return;}
//    if(startsWith(info, "connect to"))             {IPAddress dns1(8, 8, 8, 8); IPAddress dns2(8, 8, 4, 4); WiFi.setDNS(dns1, dns2);}
    llog_i("AUDIO_info:  %s", String(info).substring(0, 100).c_str());
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_showstation(const char* info) {
    if(!info) return;
    x_ps_free(&stationName_air);
    stationName_air = x_ps_strndup(info, 200); // set max length
    llog_d("StationName: %s", info);
    //_f_newStationName = true;
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_showstreamtitle(const char* info) {
    //strcpy(streamTitle, info);
    //if(!_f_irNumberSeen) _f_newStreamTitle = true;
    stream_title = String(info);
    llog_d("StreamTitle: %s", info);
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void show_ST_commercial(const char* info) {
    commercial_dur = atoi(info) / 1000; // info is the duration of advertising in ms
    char cdur[10];
    itoa(commercial_dur, cdur, 10);
    //if(_f_newCommercial) return;
    strcpy(commercial, "Advertising: ");
    strcat(commercial, cdur);
    strcat(commercial, "s");
    //_f_newCommercial = true;
    llog_d("StreamTitle: %s", info);
}
void audio_commercial(const char* info) { show_ST_commercial(info); }
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_eof_mp3(const char* info) { // end of mp3 file (filename)
    llog_d("End of file mp3: %s", info);
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_eof_stream(const char* info) {
    //_f_isWebConnected = false;
    llog_w("End of stream: %s", String(info).substring(0, 100).c_str());
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_lasthost(const char* info) { // really connected URL
    //if(_f_playlistEnabled) return;
    //x_ps_free(&_settings.lastconnectedhost);
    //_settings.lastconnectedhost = x_ps_strdup(info);
    llog_i("lastURL: ..  %s", info);
    //webSrv.send("stationURL=", _settings.lastconnectedhost);
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_icyurl(const char* info) { // if the Radio has a homepage, this event is calling
    if(strlen(info) > 5) {
        llog_d("icy-url: ..  %s", info);
    }
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_icylogo(const char* info) { // if the Radio has a homepage, this event is calling
    if(strlen(info) > 5) { llog_d("icy-logo:  %s", info); }
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_id3data(const char* info) { llog_i("id3data: %s", info); }
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_id3image(File& audiofile, const size_t APIC_pos, const size_t APIC_size) { llog_i("CoverImage:  Position %i, Size %i bytes", APIC_pos, APIC_size); }
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_oggimage(File& audiofile, std::vector<uint32_t> vec) { // OGG blockpicture
    llog_i("oggimage:..  ---------------------------------------------------------------------------");
    llog_i("oggimage:..  ogg metadata blockpicture found:");
    for(int i = 0; i < vec.size(); i += 2) { llog_i("oggimage:..  segment %02i, pos %07ld, len %05ld", i / 2, (long unsigned int)vec[i], (long unsigned int)vec[i + 1]); }
    llog_i("oggimage:..  ---------------------------------------------------------------------------");
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_icydescription(const char* info) {
    strcpy(icyDescription, info);
    //_f_newIcyDescription = true;
    if(strlen(info)) llog_d("icy-descr: %s", info);
}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_bitrate(const char* info) {
    if(!strlen(info)) return; // guard
    icyBitRate = str2int(info) / 1000;
    //_f_newBitRate = true;
    llog_d("bitRate: %i Kbit/s", icyBitRate);
}