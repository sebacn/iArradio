#ifndef _controls_h
#define _controls_h

#include <Arduino.h>
#include "icy_stream.hpp"
#include "epaper.hpp"
#include "translations.hpp"

void power_off();
void low_battery();
void set_updating(bool value);

#endif