#include "rtc_night_alarm.h"
#include "rtc_config.h"

extern RTC_DS3231 rtc;

static bool isNight(int h) { return (h >= 22 || h < 5); }

bool rtc_isNightTime() {
    DateTime now = rtc.now();
    if (now.year() < 2020) { Serial.println("WARN: RTC not set – skipping night check"); return false; }
    int h = now.hour();
    Serial.printf("RTC time: %02d:%02d:%02d\n", h, now.minute(), now.second());
    return isNight(h);
}

static void setAlarmForNext5AM() {
    DateTime now = rtc.now();
    DateTime target(now.year(), now.month(), now.day(), 5, 0, 0);
    if (now >= target) target = target + TimeSpan(1, 0, 0, 0);
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    rtc.writeSqwPinMode(DS3231_OFF);
    rtc.setAlarm1(target, DS3231_A1_Hour);
    Serial.printf("Night alarm set for: %04d-%02d-%02d 05:00:00\n",target.year(), target.month(), target.day());
}

void rtc_nightSleepTo5am() {
    Serial.println("Night window – sleeping until 05:00");
    setAlarmForNext5AM();
    pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)CLOCK_INTERRUPT_PIN, 0);
    Serial.flush();
    esp_deep_sleep_start();
}
