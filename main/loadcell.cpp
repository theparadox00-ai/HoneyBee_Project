#include "loadcell.h"
#include <SD.h>

HX711 scale;

static void _writeOffsetToSD(long offset) {
    if (!SD.exists("/LoadCell")) SD.mkdir("/LoadCell");
    File f = SD.open(OFFSET_FILE, FILE_WRITE);
    if (!f) {
        Serial.println("[LC] ERR: cannot write offset file");
        return;
    }
    f.println(offset);
    f.flush();
    f.close();
    Serial.printf("[LC] Offset saved to SD: %ld\n", offset);
}

static bool _readOffsetFromSD(long* out) {
    if (!SD.exists(OFFSET_FILE)) return false;
    File f = SD.open(OFFSET_FILE, FILE_READ);
    if (!f || f.size() == 0) {
        if (f) f.close();
        return false;
    }
    char buf[32];
    size_t len = f.readBytesUntil('\n', buf, sizeof(buf) - 1);
    f.close();
    buf[len] = '\0';
    if (len == 0) return false;
    *out = atol(buf);
    Serial.printf("[LC] Offset loaded from SD: %ld\n", *out);
    return true;
}

bool LoadCell_ReadOffsetFromSD(long* out) {
    return _readOffsetFromSD(out);
}

void LoadCell_Init(long& savedOffset, bool offsetReady) {
    Serial.println("[LC] Initialising load cell...");
    scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);

    if (offsetReady) {
        Serial.printf("[LC] Using RTC RAM offset (no SD read): %ld\n", savedOffset);
        scale.set_offset(savedOffset);
        scale.set_scale(CALIBRATION_FACTOR);

    } else {
        long sdOffset = 0;
        bool haveOffset = _readOffsetFromSD(&sdOffset);

        if (haveOffset) {
            Serial.println("[LC] Using stored offset from SD (no tare).");
            scale.set_offset(sdOffset);
            scale.set_scale(CALIBRATION_FACTOR);
            savedOffset = sdOffset;
        } else {
            Serial.println("[LC] No offset found – performing tare (first boot)...");
            Serial.println("Please remove the weights");
            scale.tare();
            delay(10000);
            Serial.println("[LC] 10 s delay complete – tare done.");
            long rawOffset = scale.read_average(10);
            Serial.printf("[LC] Raw offset: %ld\n", rawOffset);
            scale.set_offset(rawOffset);
            scale.set_scale(CALIBRATION_FACTOR);
            savedOffset = rawOffset;
            _writeOffsetToSD(savedOffset);
            Serial.println("[LC] Offset stored to SD.");
        }
    }
}

float LoadCell_Read() {
    if (!scale.is_ready()) {
        Serial.println("[LC] WARN: HX711 not ready");
        scale.power_down();
        return 0.0f;
    }

    float weight = scale.get_units(5);
    scale.power_down();
    Serial.printf("[LC] Weight: %.2f g\n", weight);
    return weight;
}

void LoadCell_Sleep() {
    scale.power_down();
    Serial.println("[LC] Load cell powered down.");
}
