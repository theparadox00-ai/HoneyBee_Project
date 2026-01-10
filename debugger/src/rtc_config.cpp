#include "rtc_config.h"

TwoWire I2C_RTC = TwoWire(1);
RTC_DS3231 rtc; 

bool initRTC() {
    I2C_RTC.begin(17, 16); 
    if (!rtc.begin(&I2C_RTC)) {
        return false;
    }
    return true;
}

char* Time_(char* buffer) {
    DateTime now = rtc.now();
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
    return buffer;
}

bool clock_Synchronization() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10000)) {
        return false;
    }
    rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    return true;
}

void Alarm_() {
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    rtc.writeSqwPinMode(DS3231_OFF);

    DateTime now = rtc.now();
    DateTime target = now + TimeSpan(SLEEP_DURATION); 
    rtc.setAlarm1(target, DS3231_A1_Second); 

    esp_sleep_enable_ext0_wakeup((gpio_num_t)CLOCK_INTERRUPT_PIN, 0); 
    
    Serial.println("Alarm set for +20s");
}
