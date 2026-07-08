#include "config.h"
#include "loadcell.h"
#include "temp_hum.h"
#include "rtc_config.h"
#include "sd_card.h"
#include "file.h"
#include "transmission.h"
#include "audio.h"
#include "scheduler.h"
#include <RTClib.h>
#include <vector>
#include <Wire.h>

extern RTC_DS3231 rtc;

RTC_DATA_ATTR long   savedOffset  = 0;     
RTC_DATA_ATTR bool   offsetReady  = false;
RTC_DATA_ATTR size_t rtcBootCount = 0;     
RTC_DATA_ATTR bool   rtcBCReady   = false; 

T_S_sensor   SHT45;
SFE_MAX1704X lipo;

bool  g_lipoOK      = false;
float g_battSOC     = -1.0f;
float g_battVoltage = -1.0f;

enum class TaskInterval : uint16_t {
    AUDIO     = AUDIO_INTERVAL,
    LOAD_CELL = LC_INTERVAL,
    SHT45     = SHT_INTERVAL,
    EMAIL     = EMAIL_INTERVAL
};

constexpr size_t Audio_Int    = static_cast<size_t>(TaskInterval::AUDIO);
constexpr size_t LoadCell_Int = static_cast<size_t>(TaskInterval::LOAD_CELL);
constexpr size_t SHT45_Int    = static_cast<size_t>(TaskInterval::SHT45);
constexpr size_t Email_Int    = static_cast<size_t>(TaskInterval::EMAIL);

static void readBatterySafe() {
    g_lipoOK = lipo.begin();
    if (!g_lipoOK) {
        g_battSOC     = -1.0f;
        g_battVoltage = -1.0f;
        Serial.println("WARN: MAX17043 not found – battery readings unavailable");
        return;
    }
    g_battSOC     = (float)lipo.getSOC();
    g_battVoltage = (float)lipo.getVoltage();
    Serial.printf("Battery: %.1f%% / %.3fV\n", g_battSOC, g_battVoltage);
}

static size_t readBootCountSafe() {
    size_t sdCount = 0;
    Serial.println("    RTC RAM cold-start - reading boot count from SD...");
    sdCount = BootCount(DIR_BC, "/count.txt");
    if (sdCount == (size_t)-1 || sdCount > 100000000UL) {
        Serial.println("    Boot count missing/invalid on SD - starting from 0");
        sdCount = 0;
        writeBootCount(DIR_BC, "/count.txt", sdCount);
        Serial.println("    Created /BootCount/count.txt with 0");
    }
    return sdCount;
}

class ScheduledTask {
public:
    virtual bool shouldExecute(size_t bootCount) = 0;
    virtual void execute(char* timeStr, long& offset, bool& offReady) = 0;
    virtual ~ScheduledTask() {}
};

class AudioTask : public ScheduledTask {
public:
    size_t _bootCount = 0;
    bool shouldExecute(size_t bootCount) override {
        return (bootCount % Audio_Int) == 0 && bootCount != 0;
    }
    void execute(char* /*timeStr*/, long& /*offset*/, bool& /*offReady*/) override {
        Serial.println(">> AudioTask");
        DateTime now = rtc.now();
        init_i2s_mic();
        bool isEmailBoot = (_bootCount % Email_Int) == 0 && _bootCount != 0;
        Serial.println(isEmailBoot
            ? "   [Audio] Email boot - recording to permanent + TX folder."
            : "   [Audio] Non-email boot - recording to permanent folder only.");
        record_wav_file(now, DIR_AUDIO, isEmailBoot);
        deinit_i2s_mic();
    }
};

class LoadCellTask : public ScheduledTask {
public:
    size_t _bootCount = 0;
    bool shouldExecute(size_t bootCount) override {
        return (bootCount % LoadCell_Int) == 0;
    }
    void execute(char* timeStr, long& offset, bool& offReady) override {
        Serial.println(">> LoadCellTask");
        unsigned long starttime = millis();
        LoadCell_Init(offset, offReady);
        delay(500);
        float weight = LoadCell_Read();
        if (!offReady && offset != 0) {
            offReady = true;
            Serial.printf("   [LC] offsetReady set to true - offset: %ld\n", offset);
        }
        unsigned long endtime = millis();
        Serial.printf("   Weight: %.2f g\n", weight);
        if (_bootCount == 0) {
            Serial.println("   [Boot 0] Tare only - skipping SD write.");
        } else {
            Serial.printf("   Time it took for execution: %lu ms\n", endtime - starttime);
            LoadCell_write(timeStr, weight);
        }
        LoadCell_Sleep();
    }
};

class SHT45Task : public ScheduledTask {
public:
    bool shouldExecute(size_t bootCount) override {
        return (bootCount % SHT45_Int) == 0 && bootCount != 0;
    }
    void execute(char* timeStr, long& /*offset*/, bool& /*offReady*/) override {
        Serial.println(">> SHT45Task");
        SHT45.init();
        float temp = -999.0f, humi = -999.0f;
        SHT45.readTempHum(temp, humi);
        Serial.printf("   Temp: %.2f C  Humi: %.2f %%\n", temp, humi);
        if (temp != -999.0f && humi != -999.0f) {
            SHT_write(timeStr, temp, humi);
        } else {
            Serial.println("   SHT45 read invalid - skipping SD write");
        }
    }
};

class WiFiEmailTask : public ScheduledTask {
public:
    bool shouldExecute(size_t bootCount) override {
        return (bootCount % Email_Int) == 0 && bootCount != 0;
    }
    void execute(char* /*timeStr*/, long& /*offset*/, bool& /*offReady*/) override {
        Serial.println(">> WiFiEmailTask");
        if (g_lipoOK) {
            Serial.printf("   Battery: %.1f%% / %.2fV\n", g_battSOC, g_battVoltage);
        } else {
            Serial.println("   Battery gauge unavailable");
        }
        Serial.print("   Connecting WiFi");
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < 25000) {
            delay(500); Serial.print(".");
        }
        Serial.println();
        if (WiFi.status() == WL_CONNECTED) {
            clock_Synchronization();
            int socInt = g_lipoOK ? (int)g_battSOC : -1;
            float volt = g_lipoOK ? g_battVoltage : -1.0f;
            if (Send_All_Data_Email(socInt, volt)) {
                Serial.println("   Email + audio sent OK - TX cleared");
            } else {
                Serial.println("   Email FAILED - TX kept for next cycle");
            }
        } else {
            Serial.println("   WiFi FAILED - skipping email this cycle");
        }
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }
};

void setup() {
    Serial.begin(115200);
    delay(2000);
    unsigned long clock_1 = millis();

    pinMode(MOSFET_PIN, OUTPUT);
    pinMode(SD_CS_PIN,  OUTPUT);
    digitalWrite(MOSFET_PIN, HIGH);
    delay(100);

    Serial.println("BEEHIVE MONITOR  (Integrated)");

    Serial.println("[1] RTC...");
    Wire.begin();

    bool rtcOK = initRTC();
    if (!rtcOK) {
        Serial.println("ERR: DS3231 not found - timer sleep only");
    } else {
        Serial.println("DS3231 OK");
    }

    bool rtcValid = rtcOK ? rtc_syncIfNeeded() : false;

    char timeStr[25];
    if (rtcOK) {
        Time_(timeStr);
        Serial.print("Time: ");
        Serial.println(timeStr);
        if (!rtcValid) {
            Serial.println("WARN: RTC invalid (no WiFi?). Will retry next boot.");
            Serial.println("Check for connection");
            foreverSleep();
        }
    } else {
        snprintf(timeStr, sizeof(timeStr), "0000-00-00 00:00:00");
    }

    Serial.println("[2] Fuel gauge...");
    readBatterySafe();

    if (g_lipoOK && g_battVoltage <= 3.600f) {
        Serial.println("CRITICAL: Battery <= 3.600V!");
        Send_LowBattery_Email((int)g_battSOC, g_battVoltage);
        foreverSleep();
    }

    Serial.println("[3] SD card...");
    if (!Init_SD()) {
        Serial.println("    ERR: SD failed - sleeping then retrying");
        SD_Sleep();
        digitalWrite(MOSFET_PIN, LOW);
        digitalWrite(SD_CS_PIN, LOW);
        Serial.println("No SD Mounted");
        foreverSleep();
    }

    Serial.println("[4] Scheduler check...");
    if (rtcOK && rtcValid) {
        scheduler_check_and_sleep_if_night();
        Serial.println("Daytime - continuing.");
        scheduler_print_daytime_window();
    } else {
        Serial.println("Skipping scheduler (RTC not valid) - assuming daytime.");
    }

    Serial.println("[5] Boot counter...");
    size_t bootCount = 0;

    if (!rtcBCReady) {
        rtcBootCount = readBootCountSafe();
        rtcBCReady   = true;
        Serial.printf("    Restored boot count: #%zu\n", rtcBootCount);
    } else {
        Serial.printf("    Using RTC RAM boot count: #%zu\n", rtcBootCount);
    }

    bootCount = rtcBootCount;
    Serial.printf("Boot #%zu\n", bootCount);
    Serial.println("[6] Load cell offset...");
    if (!offsetReady) {
        long sdOffset = 0;
        if (LoadCell_ReadOffsetFromSD(&sdOffset)) {
            savedOffset = sdOffset;
            offsetReady = true;
            Serial.printf("    Offset restored from SD into RTC RAM: %ld\n", savedOffset);
        } else {
            Serial.println("    No offset on SD yet - will tare on first boot.");
        }
    } else {
        Serial.printf("    Using RTC RAM offset: %ld\n", savedOffset);
    }

    if (g_lipoOK) {
        Serial.printf("[7] Battery: %.1f%% / %.2fV\n", g_battSOC, g_battVoltage);
    } else {
        Serial.println("[7] Battery: unavailable");
    }

    Serial.println("\n========================================");
    Serial.printf("  BOOT #%zu\n", bootCount);
    Serial.println("========================================");
    Serial.println("[8] Running scheduled tasks...");

    if (rtcOK) { Time_(timeStr); }

    AudioTask     audioTask; audioTask._bootCount = bootCount;
    LoadCellTask  lcTask;    lcTask._bootCount    = bootCount;
    SHT45Task     shtTask;
    WiFiEmailTask emailTask;

    if (bootCount == 0) {
        Serial.println("[8] Boot 0 - tare only.");
        lcTask.execute(timeStr, savedOffset, offsetReady);
    } else {
        std::vector<ScheduledTask*> tasks = {
            &audioTask, &lcTask, &shtTask, &emailTask
        };
        for (auto* t : tasks) {
            if (t && t->shouldExecute(bootCount)) {
                t->execute(timeStr, savedOffset, offsetReady);
            }
        }
    }
    rtcBootCount++;
    writeBootCount(DIR_BC, "/count.txt", rtcBootCount);
    Serial.printf("[9] Next boot #%zu written to SD and RTC RAM.\n", rtcBootCount);
    SD_Sleep();
    digitalWrite(MOSFET_PIN, LOW);
    digitalWrite(SD_CS_PIN, LOW);

    Serial.println("[10] Entering deep sleep...");
    Serial.println("========================================\n");
    Serial.flush();

    goToSleep(SLEEP_DURATION);
}

void loop() { }
