#ifndef _wifi_hardware_h
#define _wifi_hardware_h

#include <WiFi.h>
#include <WiFiManager.h>
#include "epaper.hpp"
#include "translations.hpp"

void init_wifi();
int get_wifi_rssi();
void configModeCallback(WiFiManager *myWiFiManager);
void wifi_rutine();

#endif