#ifndef SD_CARD_H
#define SD_CARD_H

#include "config.h"

// ── Permanent log directories ────────────────────────────────
extern const char* DIR_LC;       // /LoadCell
extern const char* DIR_SHT;      // /SHT45
extern const char* DIR_BC;       // /BootCount
extern const char* DIR_AUDIO;    // /Audio        (permanent WAV archive)

// ── TX buffer directory (cleared after every successful email) ─
extern const char* DIR_TX;       // /Transmit
extern const char* DIR_AUDIO_TX; // /Transmit/Audio  (WAVs sent then deleted)

// ── Permanent CSV paths ──────────────────────────────────────
extern const char* LC_PATH;      // /LoadCell/data.csv
extern const char* SHT_PATH;     // /SHT45/data.csv

// ── TX CSV buffer paths ──────────────────────────────────────
extern const char* LC_TX_PATH;   // /Transmit/lc_tx.csv
extern const char* SHT_TX_PATH;  // /Transmit/sht_tx.csv

// ── Functions ────────────────────────────────────────────────
bool   Init_SD();
void   LoadCell_write(char* timestamp, float load);
void   SHT_write(char* timestamp, float temp, float humi);

// WAV recording: saves to both /Audio and /Transmit/Audio
void   record_wav_file(size_t bootCount);

// Write WAV header to an open File object
void   write_wav_header(File file, uint32_t data_size);

// Clears TX CSV buffers and TX audio folder after successful email
void   clearTxFiles();

void   SD_Sleep();

#endif
