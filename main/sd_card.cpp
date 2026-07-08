#include "sd_card.h"
#include "audio.h"
#include <SD.h>
#include <SPI.h>

SPIClass spiSD(1);

const char* DIR_LC        = "/LoadCell";
const char* DIR_SHT       = "/SHT45";
const char* DIR_BC        = "/BootCount";
const char* DIR_AUDIO     = "/Audio";
const char* DIR_TX        = "/Transmit";
const char* DIR_AUDIO_TX  = "/Transmit/Audio";

const char* LC_PATH       = "/LoadCell/data.csv";
const char* SHT_PATH      = "/SHT45/data.csv";
const char* LC_TX_PATH    = "/Transmit/lc_tx.csv";
const char* SHT_TX_PATH   = "/Transmit/sht_tx.csv";

static const char* NIGHT_FLAG_PATH = "/BootCount/night_flag.txt";

static void createIfMissing(const char* path, const char* header) {
    if (!SD.exists(path)) {
        File f = SD.open(path, FILE_WRITE);
        if (f) {
            f.println(header);
            f.flush();
            f.close();
            Serial.print("[SD] Created: "); Serial.println(path);
        } else {
            Serial.print("[SD] ERR create: "); Serial.println(path);
        }
    }
}

static void buildUniqueFilename(const DateTime& dt, const char* dir, char* out, size_t outSize) {
    char base[32];
    snprintf(base, sizeof(base), "%04d%02d%02d_%02d%02d%02d",
             dt.year(), dt.month(), dt.day(),
             dt.hour(), dt.minute(), dt.second());

    snprintf(out, outSize, "%s/%s.wav", dir, base);
    if (!SD.exists(out)) return;

    for (int i = 1; i <= 999; i++) {
        snprintf(out, outSize, "%s/%s_%03d.wav", dir, base, i);
        if (!SD.exists(out)) return;
    }
    snprintf(out, outSize, "%s/%s_999.wav", dir, base);
}

bool Init_SD() {
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);

    spiSD.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    if (!SD.begin(SD_CS_PIN, spiSD)) {
        Serial.println("[SD] ERR: SD.begin() failed");
        return false;
    }

    const char* dirs[] = { DIR_LC, DIR_SHT, DIR_BC,
                           DIR_AUDIO, DIR_TX, DIR_AUDIO_TX };
    for (auto d : dirs) {
        if (!SD.exists(d)) SD.mkdir(d);
    }

    createIfMissing(LC_PATH,     "Timestamp,Weight_g");
    createIfMissing(SHT_PATH,    "Timestamp,Temperature_C,Humidity_pct");
    createIfMissing(LC_TX_PATH,  "Timestamp,Weight_g");
    createIfMissing(SHT_TX_PATH, "Timestamp,Temperature_C,Humidity_pct");

    Serial.println("[SD] Init OK");
    Serial.println("[SD] Layout:");
    Serial.println("  /LoadCell/data.csv              permanent load-cell log");
    Serial.println("  /LoadCell/offset.txt            HX711 tare offset");
    Serial.println("  /SHT45/data.csv                 permanent SHT45 log");
    Serial.println("  /Audio/YYYYMMDD_HHMMSS.wav      permanent WAV archive");
    Serial.println("  /Transmit/lc_tx.csv             TX buffer");
    Serial.println("  /Transmit/sht_tx.csv            TX buffer");
    Serial.println("  /Transmit/Audio/                TX WAVs");
    Serial.println("  /BootCount/count.txt            boot counter");
    Serial.println("  /BootCount/night_flag.txt       night-sleep flag");
    return true;
}

uint8_t readNightSleepFlag() {
    if (!SD.exists(NIGHT_FLAG_PATH)) {
        Serial.println("[SD] night_flag.txt missing – defaulting to 0");
        return 0;
    }

    File f = SD.open(NIGHT_FLAG_PATH, FILE_READ);
    if (!f) {
        Serial.println("[SD] ERR: cannot open night_flag.txt for read – defaulting to 0");
        return 0;
    }

    uint8_t val = 0;
    if (f.available()) {
        val = (uint8_t)(f.parseInt());  
    }
    f.close();

    Serial.printf("[SD] Read night_flag = %u\n", val);
    return val;
}

void writeNightSleepFlag(uint8_t val) {
    SD.remove(NIGHT_FLAG_PATH);          
    File f = SD.open(NIGHT_FLAG_PATH, FILE_WRITE);
    if (!f) {
        Serial.println("[SD] ERR: cannot open night_flag.txt for write");
        return;
    }
    f.println(val);
    f.flush();
    f.close();
    Serial.printf("[SD] Wrote night_flag = %u\n", val);
}

void LoadCell_write(char* timestamp, float load) {
    File f1 = SD.open(LC_PATH, FILE_APPEND);
    if (f1) {
        f1.print(timestamp); f1.print(","); f1.println(load, 2);
        f1.flush(); f1.close();
    } else { Serial.println("[SD] ERR: LC permanent write failed"); }

    File f2 = SD.open(LC_TX_PATH, FILE_APPEND);
    if (f2) {
        f2.print(timestamp); f2.print(","); f2.println(load, 2);
        f2.flush(); f2.close();
        Serial.println("[SD] LC data saved.");
    } else { Serial.println("[SD] ERR: LC TX write failed"); }
}

void SHT_write(char* timestamp, float temp, float humi) {
    File f1 = SD.open(SHT_PATH, FILE_APPEND);
    if (f1) {
        f1.print(timestamp); f1.print(",");
        f1.print(temp, 2); f1.print(","); f1.println(humi, 2);
        f1.flush(); f1.close();
    } else { Serial.println("[SD] ERR: SHT permanent write failed"); }

    File f2 = SD.open(SHT_TX_PATH, FILE_APPEND);
    if (f2) {
        f2.print(timestamp); f2.print(",");
        f2.print(temp, 2); f2.print(","); f2.println(humi, 2);
        f2.flush(); f2.close();
        Serial.println("[SD] SHT data saved.");
    } else { Serial.println("[SD] ERR: SHT TX write failed"); }
}

void write_wav_header(File file, uint32_t data_size) {
    uint32_t file_size       = data_size + 36;
    uint16_t num_channels    = NUM_CHANNELS;
    uint32_t sample_rate     = SAMPLE_RATE;
    uint32_t byte_rate       = BYTE_RATE;
    uint16_t block_align     = NUM_CHANNELS * BYTES_PER_SAMPLE;
    uint16_t bits_per_sample = BITS_PER_SAMPLE;
    uint16_t audio_format    = 1;
    uint32_t fmt_size        = 16;

    file.write((uint8_t*)"RIFF",           4);
    file.write((uint8_t*)&file_size,       4);
    file.write((uint8_t*)"WAVE",           4);
    file.write((uint8_t*)"fmt ",           4);
    file.write((uint8_t*)&fmt_size,        4);
    file.write((uint8_t*)&audio_format,    2);
    file.write((uint8_t*)&num_channels,    2);
    file.write((uint8_t*)&sample_rate,     4);
    file.write((uint8_t*)&byte_rate,       4);
    file.write((uint8_t*)&block_align,     2);
    file.write((uint8_t*)&bits_per_sample, 2);
    file.write((uint8_t*)"data",           4);
    file.write((uint8_t*)&data_size,       4);
}

const char* record_wav_file(const DateTime& now, const char* /*saveDir*/, bool writeTx) {
    static char perm_path[64];
    static char tx_path[64];

    buildUniqueFilename(now, DIR_AUDIO,    perm_path, sizeof(perm_path));
    buildUniqueFilename(now, DIR_AUDIO_TX, tx_path,   sizeof(tx_path));

    Serial.printf("[REC] Permanent : %s\n", perm_path);
    Serial.printf("[REC] TX copy   : %s\n", tx_path);

    File perm = SD.open(perm_path, FILE_WRITE);
    File tx;
    if (writeTx) tx = SD.open(tx_path, FILE_WRITE);

    if (!perm) {
        Serial.println("[REC] ERR: cannot open permanent WAV");
        if (writeTx && tx) tx.close();
        return perm_path;
    }
    if (writeTx && !tx) {
        Serial.println("[REC] ERR: cannot open TX WAV");
        perm.close();
        return perm_path;
    }

    uint32_t total_samples = (uint32_t)SAMPLE_RATE * RECORD_TIME;
    uint32_t samples_done  = 0;

    write_wav_header(perm, WAV_DATA_SIZE);
    if (writeTx) write_wav_header(tx, WAV_DATA_SIZE);

    int32_t buf[I2S_BUFFER_SAMPLES];

    Serial.println("[REC] Recording...");
    unsigned long t0 = millis();

    while (samples_done < total_samples) {
        size_t to_read = (size_t)(total_samples - samples_done);
        if (to_read > I2S_BUFFER_SAMPLES) to_read = I2S_BUFFER_SAMPLES;

        int got = read_i2s_data(buf, to_read);
        if (got <= 0) continue;

        for (int i = 0; i < got; i++) {
            int32_t s = buf[i];
            s *= 5;
            if (s >  8388607) s =  8388607;
            if (s < -8388608) s = -8388608;

            uint8_t b[3];
            b[0] =  s        & 0xFF;
            b[1] = (s >>  8) & 0xFF;
            b[2] = (s >> 16) & 0xFF;

            perm.write(b, 3);
            if (writeTx) tx.write(b, 3);
        }

        samples_done += (uint32_t)got;

        if ((samples_done % SAMPLE_RATE) == 0) {
            Serial.printf("  %lu/%d s (%d%%)\n",
                          samples_done / SAMPLE_RATE, RECORD_TIME,
                          (int)((samples_done * 100ul) / total_samples));
        }
    }

    perm.flush(); perm.close();
    if (writeTx) { tx.flush(); tx.close(); }

    Serial.printf("[REC] Done in %.2f s  |  %s\n",
                  (millis() - t0) / 1000.0f, perm_path);
    return perm_path;
}

void clearTxFiles() {
    SD.remove(LC_TX_PATH);
    File f1 = SD.open(LC_TX_PATH, FILE_WRITE);
    if (f1) { f1.println("Timestamp,Weight_g"); f1.flush(); f1.close();
              Serial.println("[SD] LC TX CSV cleared."); }

    SD.remove(SHT_TX_PATH);
    File f2 = SD.open(SHT_TX_PATH, FILE_WRITE);
    if (f2) { f2.println("Timestamp,Temperature_C,Humidity_pct"); f2.flush(); f2.close();
              Serial.println("[SD] SHT TX CSV cleared."); }

    File dir = SD.open(DIR_AUDIO_TX);
    if (dir) {
        File entry = dir.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                char full[80];
                snprintf(full, sizeof(full), "%s/%s", DIR_AUDIO_TX, entry.name());
                entry.close();
                SD.remove(full);
                Serial.print("[SD] Deleted TX WAV: "); Serial.println(full);
            } else { entry.close(); }
            entry = dir.openNextFile();
        }
        dir.close();
    }
}

void SD_Sleep() {
    SD.end();
    spiSD.end();
    Serial.println("[SD] Card unmounted and SPI released.");
}
