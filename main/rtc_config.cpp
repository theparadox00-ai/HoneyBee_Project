#include "rtc_config.h"
 
static const char* ntpServer          = "in.pool.ntp.org";   // Indian NTP pool
// static const char* ntpServer2         = "time.google.com";   // fallback
static const long  gmtOffset_sec      = LOCATION_GMT_OFFSET_SEC;
static const int   daylightOffset_sec = 0;
 
RTC_DS3231 rtc;
 
bool initRTC() {
    if (!rtc.begin()) {
        Serial.println("[RTC] ERR: DS3231 not found on I2C bus!");
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
 
bool rtc_needsSync() {
    DateTime now = rtc.now();

    if (!now.isValid()) return true;

    if (now.year()  < 2020 || now.year()  > 2099) return true;
    if (now.month() < 1    || now.month() > 12)   return true;
    if (now.day()   < 1    || now.day()   > 31)   return true;
    if (now.hour()  > 23)                         return true;
    if (now.minute()> 59)                         return true;
    if (now.second()> 59)                         return true;

    return false;
}

bool clock_Synchronization() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);// ntpServer2
 
    Serial.print("[RTC] Waiting for NTP");
    struct tm timeinfo;
    bool got = false;
    for (int i = 0; i < 40; i++) {
        if (getLocalTime(&timeinfo, 500)) { got = true; break; }
        Serial.print(".");
    }
    Serial.println();
 
    if (!got) {
        Serial.println("[RTC] ERR: NTP sync failed");
        return false;
    }
 
    rtc.adjust(DateTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon  + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec));
 
    char buf[25];
    Serial.print("[RTC] Synced. New time: ");
    Serial.println(Time_(buf));
    return true;
}
 
bool rtc_syncIfNeeded() {
    if (!rtc_needsSync()) {
        Serial.println("[RTC] Time valid – no sync needed.");
        return true;
    }
 
    Serial.println("[RTC] Time invalid (year < 2020) – syncing via NTP...");
 
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
 
    Serial.print("[RTC] Connecting WiFi");
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 25000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
 
    bool ok = false;
    if (WiFi.status() == WL_CONNECTED) {
        ok = clock_Synchronization();
    } else {
        Serial.println("[RTC] WiFi failed – RTC remains unset. Will retry next boot.");
    }
 
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return ok;
}
 
void goToSleep(uint32_t sleep_duration) {
    esp_sleep_enable_timer_wakeup((uint64_t)sleep_duration * 1000000ULL);
    Serial.print("The system will wake up after ");
    Serial.print(sleep_duration);
    Serial.println(" seconds");
    Serial.flush();
    esp_deep_sleep_start();
}

void foreverSleep() {
    Serial.println("[BATT] Entering FOREVER deep sleep (no wake timer).");
    Serial.flush();
    SD_Sleep();
    digitalWrite(MOSFET_PIN, LOW);
    digitalWrite(SD_CS_PIN, LOW);
    esp_deep_sleep_start();
}
