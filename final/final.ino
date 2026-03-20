#include "config.h"
#include "loadcell.h"
#include "temp_hum.h"
#include "rtc_config.h"
#include "rtc_night_alarm.h"
#include "sd_card.h"
#include "file.h"
#include "transmission.h"
#include "audio.h"
#include <vector>

RTC_DATA_ATTR long savedOffset = 0;

T_S_sensor   SHT45;
SFE_MAX1704X lipo;

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

class ScheduledTask {
public:
    virtual bool shouldExecute(size_t bootCount) = 0;
    virtual void execute(char* timeStr, long& savedOffset) = 0;
    virtual ~ScheduledTask() {}
};

class AudioTask : public ScheduledTask {
public:
    bool shouldExecute(size_t bootCount) override {
        return (bootCount % Audio_Int) == 0;
    }
    void execute(char* /*timeStr*/, long& /*savedOffset*/) override {
        Serial.println(">> AudioTask");
        init_i2s_mic();
        record_wav_file(bootCount_);
        deinit_i2s_mic();
    }
    void setBootCount(size_t bc) { bootCount_ = bc; }
private:
    size_t bootCount_ = 0;
};

class LoadCellTask : public ScheduledTask {
public:
    bool shouldExecute(size_t bootCount) override {
        return (bootCount % LoadCell_Int) == 0;
    }
    void execute(char* timeStr, long& savedOffset) override {
        Serial.println(">> LoadCellTask");
        float weight = LoadCellReading((int)bootCount_, savedOffset);
        Serial.print("   Weight: "); Serial.print(weight); Serial.println(" g");
        LoadCell_write(timeStr, weight);
    }
    void setBootCount(size_t bc) { bootCount_ = bc; }
private:
    size_t bootCount_ = 0;
};

class SHT45Task : public ScheduledTask {
public:
    bool shouldExecute(size_t bootCount) override {
        return (bootCount % SHT45_Int) == 0 && bootCount != 0;
    }
    void execute(char* timeStr, long& /*savedOffset*/) override {
        Serial.println(">> SHT45Task");
        SHT45.init();
        float temp = -999.0f, humi = -999.0f;
        SHT45.readTempHum(temp, humi);
        Serial.printf("   Temp: %.2f C  Humi: %.2f %%\n", temp, humi);
        if (temp != -999.0f && humi != -999.0f) SHT_write(timeStr, temp, humi);
    }
};

class WiFiEmailTask : public ScheduledTask {
public:
    bool shouldExecute(size_t bootCount) override {
        return (bootCount % Email_Int) == 0 && bootCount != 0;
    }
    void execute(char* /*timeStr*/, long& /*savedOffset*/) override {
        Serial.println(">> WiFiEmailTask");

        float soc     = (float)lipo.getSOC();
        float voltage = (float)lipo.getVoltage();
        Serial.printf("   Battery: %.1f%% / %.2fV\n", soc, voltage);

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
            Serial.println("   WiFi connected");
            clock_Synchronization();
            if (Send_All_Data_Email((int)soc, voltage)) {
                Serial.println("   Email + audio sent OK – TX folder cleared");
            } else {
                Serial.println("   Email FAILED – TX data kept for next cycle");
            }
        } else {
            Serial.println("   WiFi FAILED – skipping email this cycle");
        }

        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }
};

void setup() {
    Serial.begin(115200);
    delay(2000);

    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, HIGH);

    Serial.println("\n========================================");
    Serial.println("      BEEHIVE MONITOR  (+ AUDIO)");
    Serial.println("========================================");

    // ── 1. RTC ──────────────────────────────────────────────
    Serial.println("[1] RTC...");
    if (!initRTC()) Serial.println("    ERR: RTC init failed!");
    else             Serial.println("    RTC OK");

    char timeStr[25];
    Time_(timeStr);
    Serial.print("    Time: "); Serial.println(timeStr);

    // ── 2. Night check ───────────────────────────────────────
    // Serial.println("[2] Night check...");
    // if (rtc_isNightTime()) {
    //     Serial.println("    Night window – sleeping until 05:00");
    //     digitalWrite(MOSFET_PIN, LOW);
    //     rtc_nightSleepTo5am();
    //     return;
    // }
    Serial.println("    Daytime – continuing");

    // ── 3. SD card ───────────────────────────────────────────
    Serial.println("[3] SD card...");
    if (!Init_SD()) {
        Serial.println("    ERR: SD failed – retrying next cycle");
        digitalWrite(MOSFET_PIN, LOW);
        Alarm_();
        esp_deep_sleep_start();
    }

    // ── 4. Boot counter ──────────────────────────────────────
    Serial.println("[4] Boot counter...");
    size_t bootCount = BootCount(DIR_BC, "/count.txt");
    if (bootCount == (size_t)-1) {
        Serial.println("    ERR: bad boot count – retrying");
        SD_Sleep();
        digitalWrite(MOSFET_PIN, LOW);
        Alarm_();
        esp_deep_sleep_start();
    }
    Serial.print("    Boot #"); Serial.println(bootCount);

    // ── 5. Fuel gauge ────────────────────────────────────────
    Serial.println("[5] Fuel gauge...");
    Wire.begin();
    if (!lipo.begin()) Serial.println("    WARN: MAX17043 not found");
    else                Serial.println("    Fuel gauge OK");

    // ── 6. First boot: tare + NTP + fuel gauge init ──────────
    if (bootCount == 0) {
        Serial.println("[6] FIRST BOOT – tare, NTP, fuel gauge");

        float w = LoadCellReading(0, savedOffset);
        Serial.print("    Tare done. Initial weight: "); Serial.println(w);

        Serial.print("    Connecting WiFi for NTP");
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
            Time_(timeStr);
            Serial.print("    Updated time: "); Serial.println(timeStr);
        } else {
            Serial.println("    WiFi failed – RTC not synced on first boot");
        }
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        lipo.quickStart();
        delay(200);
        lipo.setThreshold(10);
        Serial.println("    Fuel gauge configured (alert @ 10%)");
    } else {
        Serial.println("[6] Not first boot – skipping init");
    }

    // ── 7. Battery reading ───────────────────────────────────
    float soc     = (float)lipo.getSOC();
    float voltage = (float)lipo.getVoltage();
    Serial.printf("[7] Battery: %.1f%% / %.2fV\n", soc, voltage);

    Serial.println("\n========================================");
    Serial.printf("  BOOT #%zu\n", bootCount);
    Serial.println("========================================");

    // ── 8. Scheduled tasks ───────────────────────────────────
    Serial.println("[8] Running scheduled tasks...");
    Time_(timeStr);

    AudioTask     audioTask;   audioTask.setBootCount(bootCount);
    LoadCellTask  lcTask;      lcTask.setBootCount(bootCount);
    SHT45Task     shtTask;
    WiFiEmailTask emailTask;

    std::vector<ScheduledTask*> tasks = {
        &audioTask,
        &lcTask,
        &shtTask,
        &emailTask
    };

    for (auto* task : tasks) {
        if (task->shouldExecute(bootCount)) {
            task->execute(timeStr, savedOffset);
        }
    }

    // ── 9. Wrap up ───────────────────────────────────────────
    bootCount++;
    writeBootCount(DIR_BC, "/count.txt", bootCount);
    Serial.printf("[9] Next boot #%zu\n", bootCount);

    SD_Sleep();
    digitalWrite(MOSFET_PIN, LOW);

    Serial.println("    Entering deep sleep...");
    Serial.println("========================================\n");
    Serial.flush();

    Alarm_();
    esp_deep_sleep_start();
}

void loop() { /* never reached – deep sleep only */ }
