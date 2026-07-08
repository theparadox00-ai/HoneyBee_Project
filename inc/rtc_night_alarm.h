#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include <RTClib.h>

struct SunTimes {
    uint8_t sunrise_h;
    uint8_t sunrise_m;
    uint8_t sunset_h;
    uint8_t sunset_m;
};

extern RTC_DATA_ATTR uint8_t scheduler_night_sleep_flag;

SunTimes scheduler_get_for_date(uint8_t month, uint8_t day);

SunTimes scheduler_get(uint8_t month);

void scheduler_is_daytime();

void scheduler_print_daytime_window();

void scheduler_check_and_sleep_if_night();

#endif 
