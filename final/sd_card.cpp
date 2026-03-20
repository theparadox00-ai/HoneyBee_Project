#include "sd_card.h"
#include "audio.h"

// ── Directory paths ──────────────────────────────────────────
const char* DIR_LC        = "/LoadCell";
const char* DIR_SHT       = "/SHT45";
const char* DIR_BC        = "/BootCount";
const char* DIR_AUDIO     = "/Audio";
const char* DIR_TX        = "/Transmit";
const char* DIR_AUDIO_TX  = "/Transmit/Audio";

// ── Permanent CSV paths ──────────────────────────────────────
const char* LC_PATH       = "/LoadCell/data.csv";
const char* SHT_PATH      = "/SHT45/data.csv";

// ── TX buffer CSV paths ──────────────────────────────────────
const char* LC_TX_PATH    = "/Transmit/lc_tx.csv";
const char* SHT_TX_PATH   = "/Transmit/sht_tx.csv";

// ── Helper ───────────────────────────────────────────────────
static void createIfMissing(const char* path, const char* header) {
    if (!SD.exists(path)) {
        File f = SD.open(path, FILE_WRITE);
        if (f) { f.println(header); f.close();
                 Serial.print("Created: "); Serial.println(path); }
        else   { Serial.print("ERR create: "); Serial.println(path); }
    }
}

// ── SD Init ──────────────────────────────────────────────────
bool Init_SD() {
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("ERR: SD.begin() failed");
        return false;
    }

    // Create all directories
    if (!SD.exists(DIR_LC))       SD.mkdir(DIR_LC);
    if (!SD.exists(DIR_SHT))      SD.mkdir(DIR_SHT);
    if (!SD.exists(DIR_BC))       SD.mkdir(DIR_BC);
    if (!SD.exists(DIR_AUDIO))    SD.mkdir(DIR_AUDIO);
    if (!SD.exists(DIR_TX))       SD.mkdir(DIR_TX);
    if (!SD.exists(DIR_AUDIO_TX)) SD.mkdir(DIR_AUDIO_TX);

    // Create CSV files with headers
    createIfMissing(LC_PATH,     "Timestamp,Weight_g");
    createIfMissing(SHT_PATH,    "Timestamp,Temperature_C,Humidity_pct");
    createIfMissing(LC_TX_PATH,  "Timestamp,Weight_g");
    createIfMissing(SHT_TX_PATH, "Timestamp,Temperature_C,Humidity_pct");

    Serial.println("SD init OK");
    Serial.println("SD layout:");
    Serial.println("  /LoadCell/data.csv       <- permanent load cell log");
    Serial.println("  /SHT45/data.csv          <- permanent SHT45 log");
    Serial.println("  /Audio/                  <- permanent WAV archive");
    Serial.println("  /Transmit/lc_tx.csv      <- TX buffer (cleared after email)");
    Serial.println("  /Transmit/sht_tx.csv     <- TX buffer (cleared after email)");
    Serial.println("  /Transmit/Audio/         <- TX WAVs (cleared after email)");
    Serial.println("  /BootCount/count.txt     <- boot counter");
    return true;
}

// ── CSV write: permanent + TX buffer ─────────────────────────
void LoadCell_write(char* timestamp, float load) {
    File f1 = SD.open(LC_PATH, FILE_APPEND);
    if (f1) { f1.print(timestamp); f1.print(","); f1.println(load, 2); f1.close(); }
    else      Serial.println("ERR: LC permanent write failed");

    File f2 = SD.open(LC_TX_PATH, FILE_APPEND);
    if (f2) { f2.print(timestamp); f2.print(","); f2.println(load, 2); f2.close();
              Serial.println("LC saved."); }
    else      Serial.println("ERR: LC TX write failed");
}

void SHT_write(char* timestamp, float temp, float humi) {
    File f1 = SD.open(SHT_PATH, FILE_APPEND);
    if (f1) { f1.print(timestamp); f1.print(","); f1.print(temp, 2);
              f1.print(","); f1.println(humi, 2); f1.close(); }
    else      Serial.println("ERR: SHT permanent write failed");

    File f2 = SD.open(SHT_TX_PATH, FILE_APPEND);
    if (f2) { f2.print(timestamp); f2.print(","); f2.print(temp, 2);
              f2.print(","); f2.println(humi, 2); f2.close();
              Serial.println("SHT saved."); }
    else      Serial.println("ERR: SHT TX write failed");
}

// ── WAV header writer ─────────────────────────────────────────
void write_wav_header(File file, uint32_t data_size) {
    uint32_t file_size      = data_size + 36;
    uint16_t num_channels   = NUM_CHANNELS;
    uint32_t sample_rate    = SAMPLE_RATE;
    uint32_t byte_rate      = BYTE_RATE;
    uint16_t block_align    = NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
    uint16_t bits_per_smpl  = BITS_PER_SAMPLE;
    uint16_t audio_format   = 1;   // PCM
    uint32_t fmt_size       = 16;

    file.write((uint8_t*)"RIFF", 4);
    file.write((uint8_t*)&file_size,     4);
    file.write((uint8_t*)"WAVE",         4);
    file.write((uint8_t*)"fmt ",         4);
    file.write((uint8_t*)&fmt_size,      4);
    file.write((uint8_t*)&audio_format,  2);
    file.write((uint8_t*)&num_channels,  2);
    file.write((uint8_t*)&sample_rate,   4);
    file.write((uint8_t*)&byte_rate,     4);
    file.write((uint8_t*)&block_align,   2);
    file.write((uint8_t*)&bits_per_smpl, 2);
    file.write((uint8_t*)"data",         4);
    file.write((uint8_t*)&data_size,     4);
}

// ── WAV recording: saves to BOTH /Audio and /Transmit/Audio ──
void record_wav_file(size_t bootCount) {
    char perm_path[48];
    char tx_path[48];
    snprintf(perm_path, sizeof(perm_path), "/Audio/rec_%05zu.wav",  bootCount);
    snprintf(tx_path,   sizeof(tx_path),   "/Transmit/Audio/rec_%05zu.wav", bootCount);

    Serial.printf("[REC] Recording: %s\n", perm_path);

    File perm = SD.open(perm_path, FILE_WRITE);
    File tx   = SD.open(tx_path,   FILE_WRITE);

    if (!perm) { Serial.println("[REC] ERR: cannot open permanent WAV file"); return; }
    if (!tx)   { Serial.println("[REC] ERR: cannot open TX WAV file"); perm.close(); return; }

    uint32_t data_size      = WAV_DATA_SIZE;
    uint32_t total_samples  = SAMPLE_RATE * RECORD_TIME;
    uint32_t samples_done   = 0;

    write_wav_header(perm, data_size);
    write_wav_header(tx,   data_size);

    int16_t buffer[I2S_BUFFER_SAMPLES];

    Serial.println("[REC] Recording in progress...");
    unsigned long t0 = millis();

    while (samples_done < total_samples) {
        size_t to_read = min((size_t)(total_samples - samples_done),
                             (size_t)I2S_BUFFER_SAMPLES);
        read_i2s_data(buffer, to_read);

        size_t bw1 = perm.write((uint8_t*)buffer, to_read * sizeof(int16_t));
        size_t bw2 = tx.write((uint8_t*)buffer,   to_read * sizeof(int16_t));

        if (bw1 != to_read * sizeof(int16_t))
            Serial.printf("[REC] WARN: perm write mismatch (%d/%d)\n", bw1, to_read * 2);
        if (bw2 != to_read * sizeof(int16_t))
            Serial.printf("[REC] WARN: TX write mismatch (%d/%d)\n",   bw2, to_read * 2);

        samples_done += to_read;

        if (samples_done % SAMPLE_RATE == 0) {
            int secs = samples_done / SAMPLE_RATE;
            Serial.printf("  %d/%d sec (%d%%)\n", secs, RECORD_TIME,
                          (samples_done * 100) / total_samples);
        }
    }

    perm.close();
    tx.close();

    unsigned long elapsed = millis() - t0;
    Serial.printf("[REC] Done! %.2f sec  |  permanent: %s  |  TX: %s\n",
                  elapsed / 1000.0f, perm_path, tx_path);
}

// ── Clear TX buffers + TX audio folder after email ───────────
void clearTxFiles() {
    // Clear CSV TX buffers (re-create with headers)
    SD.remove(LC_TX_PATH);
    File f1 = SD.open(LC_TX_PATH, FILE_WRITE);
    if (f1) { f1.println("Timestamp,Weight_g"); f1.close();
              Serial.println("LC TX CSV cleared."); }

    SD.remove(SHT_TX_PATH);
    File f2 = SD.open(SHT_TX_PATH, FILE_WRITE);
    if (f2) { f2.println("Timestamp,Temperature_C,Humidity_pct"); f2.close();
              Serial.println("SHT TX CSV cleared."); }

    // Delete all WAV files in /Transmit/Audio/
    File dir = SD.open(DIR_AUDIO_TX);
    if (dir) {
        File entry = dir.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                char full_path[64];
                snprintf(full_path, sizeof(full_path), "%s/%s",
                         DIR_AUDIO_TX, entry.name());
                entry.close();
                SD.remove(full_path);
                Serial.print("Deleted TX WAV: "); Serial.println(full_path);
            } else {
                entry.close();
            }
            entry = dir.openNextFile();
        }
        dir.close();
        Serial.println("TX Audio folder cleared.");
    }
}

// ── Safely unmount SD ─────────────────────────────────────────
void SD_Sleep() {
    SD.end();
    Serial.println("SD unmounted.");
}
