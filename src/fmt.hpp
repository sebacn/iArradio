#ifndef _fmt_h
#define _fmt_h

#include <Arduino.h>

String capitalize(String str);
String right_pad(String text, const int size, char pad_char=' ');
String left_pad(String text, const int size, char pad_char=' ');
String fmt_2f1(float f);
String header_datetime(time_t* dt, bool updated);
String ts2weekday(int timestamp);
String ts2date(int timestamp);
String ts2H(int timestamp);
String ts2HM(int timestamp);
String ts2dm(int timestamp);

#endif
