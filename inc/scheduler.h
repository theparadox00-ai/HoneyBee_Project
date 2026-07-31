#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include <RTClib.h>

typedef struct {
    uint8_t sunrise_h;
    uint8_t sunrise_m;
    uint8_t sunset_h;
    uint8_t sunset_m;
} SunTimes;

SunTimes scheduler_get(uint8_t month);
SunTimes scheduler_get_for_date(uint8_t month, uint8_t day);
void scheduler_print_daytime_window(void);
void scheduler_check_and_sleep_if_night(void);

#endif
