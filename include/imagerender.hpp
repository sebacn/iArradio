#ifndef _imagerender_h
#define _imagerender_h

#include <Arduino.h>
#include <GxEPD2_4G_4G.h>
#include <GxEPD2_4G_BW.h>

void render4GJpegFile(const char *filename, GxEPD2_4G_4G<GxEPD2_270, GxEPD2_270::HEIGHT> * display, uint16_t x, uint16_t y, uint16_t img_w, uint16_t img_h);

#endif