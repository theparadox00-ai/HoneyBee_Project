#include "file.h"

size_t BootCount(const char* idxDir, const char* idxFile) {
    char path[64];
    snprintf(path, sizeof(path), "%s%s", idxDir, idxFile);

    if (!SD.exists(path)) { Serial.println("Boot count file not found – starting at 0"); return 0; }

    File f = SD.open(path, FILE_READ);
    if (!f) { Serial.println("ERR: could not open boot count file"); return (size_t)-1; }
    if (f.size() == 0) { f.close(); Serial.println("Boot count empty – starting at 0"); return 0; }

    char line[32];
    size_t len = f.readBytesUntil('\n', line, sizeof(line) - 1);
    line[len] = '\0';
    f.close();

    if (len == 0) { Serial.println("ERR: boot count line empty"); return (size_t)-1; }

    long val = atol(line);
    if (val == 0 && line[0] != '0') {
        Serial.print("ERR: invalid boot count: "); Serial.println(line);
        return (size_t)-1;
    }
    return (size_t)val;
}

void writeBootCount(const char* idxDir, const char* idxFile, size_t count) {
    char path[64];
    snprintf(path, sizeof(path), "%s%s", idxDir, idxFile);

    File f = SD.open(path, FILE_WRITE);
    if (!f) { Serial.println("ERR: could not write boot count file"); return; }

    char buf[16];
    snprintf(buf, sizeof(buf), "%zu\n", count);
    f.print(buf);
    f.flush();
    f.close();
    Serial.print("Boot count written: "); Serial.println(count);
}
