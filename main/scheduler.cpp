#include "scheduler.h"
#include "rtc_config.h"
#include "config.h"
#include "transmission.h"
#include "sd_card.h"

extern RTC_DS3231 rtc;
extern bool  g_lipoOK;
extern float g_battSOC;
extern float g_battVoltage;

static SunTimes s_table[13] = {
    { 6, 41, 18,  5 },  // Jan  1
    { 6, 47, 18, 22 },  // Feb  1
    { 6, 40, 18, 28 },  // Mar  1
    { 6, 20, 18, 28 },  // Apr  1
    { 6,  3, 18, 33 },  // May  1
    { 5, 54, 18, 42 },  // Jun  1
    { 5, 55, 18, 47 },  // Jul  1
    { 6,  3, 18, 43 },  // Aug  1
    { 6,  7, 18, 25 },  // Sep  1
    { 6,  5, 18,  3 },  // Oct  1
    { 6,  9, 17, 44 },  // Nov  1
    { 6, 22, 17, 41 },  // Dec  1
    { 6, 41, 18,  5 },  // Jan  1 next year (for Dec → Jan interpolation)
};

static const uint8_t s_days_in_month[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int16_t _sunriseMin(const SunTimes& t) {
    return (int16_t)t.sunrise_h * 60 + t.sunrise_m;
}
static int16_t _sunsetMin(const SunTimes& t) {
    return (int16_t)t.sunset_h * 60 + t.sunset_m;
}
static int16_t _lerp(int16_t a, int16_t b, float frac) {
    return (int16_t)(a + (b - a) * frac + 0.5f);
}

SunTimes scheduler_get_for_date(uint8_t month, uint8_t day) {
    if (month < 1 || month > 12) month = 1;

    int cur = (int)month - 1;
    int nxt = (int)month;

    uint8_t dim = s_days_in_month[cur];
    float frac = (float)(day - 1) / (float)dim;
    if (frac > 1.0f) frac = 1.0f;
    if (frac < 0.0f) frac = 0.0f;

    int16_t sr = _lerp(_sunriseMin(s_table[cur]), _sunriseMin(s_table[nxt]), frac);
    int16_t ss = _lerp(_sunsetMin (s_table[cur]), _sunsetMin (s_table[nxt]), frac);

    SunTimes result;
    result.sunrise_h = (uint8_t)(sr / 60);
    result.sunrise_m = (uint8_t)(sr % 60);
    result.sunset_h  = (uint8_t)(ss / 60);
    result.sunset_m  = (uint8_t)(ss % 60);
    return result;
}

SunTimes scheduler_get(uint8_t month) {
    if (month < 1 || month > 12) month = 1;
    return s_table[month - 1];
}

void scheduler_print_daytime_window() {
    DateTime now = rtc.now();
    if (now.year() < 2020 || now.year() > 2099) {
        Serial.println("[SCHED] WARN: RTC not set – cannot print daytime window.");
        return;
    }

    SunTimes t = scheduler_get_for_date(now.month(), now.day());

    uint16_t nowMin     = now.hour()  * 60 + now.minute();
    uint16_t sunriseMin = t.sunrise_h * 60 + t.sunrise_m;
    uint16_t sunsetMin  = t.sunset_h  * 60 + t.sunset_m;

    if (nowMin >= sunriseMin && nowMin < sunsetMin) {
        Serial.printf("[SCHED] Active window (interpolated): %02u:%02u – %02u:%02u\n",
                      t.sunrise_h, t.sunrise_m, t.sunset_h, t.sunset_m);
    }
}

void scheduler_check_and_sleep_if_night() {
    uint8_t nightFlag = readNightSleepFlag();

    if (nightFlag == 1) {
        Serial.println("[SCHED] Woke from night sleep – sending Good Morning email...");
        int socInt = g_lipoOK ? (int)g_battSOC : -1;
        float volt = g_lipoOK ? g_battVoltage : -1.0f;
        Send_WakeupEmail(socInt, volt);
        writeNightSleepFlag(0);   
        Serial.println("[SCHED] Wakeup email done. Flag cleared.");
        return;
    }

    DateTime now = rtc.now();
    SunTimes t   = scheduler_get_for_date(now.month(), now.day());

    uint16_t nowMin     = now.hour()  * 60 + now.minute();
    uint16_t sunriseMin = t.sunrise_h * 60 + t.sunrise_m;
    uint16_t sunsetMin  = t.sunset_h  * 60 + t.sunset_m;

    if (nowMin >= sunriseMin && nowMin < sunsetMin) {
        return;
    }

    DateTime sunrise_today(now.year(), now.month(), now.day(),
                           t.sunrise_h, t.sunrise_m, 0);

    DateTime target = sunrise_today;
    if (now >= sunrise_today) {
        target = sunrise_today + TimeSpan(1, 0, 0, 0);
    }

    Serial.printf("[SCHED] Night window. Sleeping until %04d-%02d-%02d %02d:%02u:00\n",
                  target.year(), target.month(), target.day(),
                  target.hour(), target.minute());

    DateTime now2 = rtc.now();

    if (target.unixtime() <= now2.unixtime()) {
        rtc_syncIfNeeded();
        now2 = rtc.now();
    }

    uint32_t seconds = (uint32_t)(target.unixtime() - now2.unixtime());

    Serial.printf("[SCHED] Sleeping %lu s until sunrise\n", (unsigned long)seconds);

    writeNightSleepFlag(1);

    int   socInt = g_lipoOK ? (int)g_battSOC   : -1;
    float volt   = g_lipoOK ?      g_battVoltage : -1.0f;
    Send_NightSleep_Email(socInt, volt);

    goToSleep(seconds);
}
