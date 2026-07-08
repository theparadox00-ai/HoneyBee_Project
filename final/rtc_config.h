#ifndef RTC_CONFIG_H
#define RTC_CONFIG_H
 
#include "config.h"
#include "sd_card.h"

bool  initRTC();
 
char* Time_(char* buffer);
 
bool  clock_Synchronization();
 
bool  rtc_needsSync();

bool  rtc_syncIfNeeded();
 
void goToSleep(uint32_t sleep_duration);

void foreverSleep();
 
#endif 
