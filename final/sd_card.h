#ifndef SD_CARD_H
#define SD_CARD_H

#include "config.h"
#include <RTClib.h>

extern const char* DIR_LC;
extern const char* DIR_SHT;
extern const char* DIR_BC;
extern const char* DIR_AUDIO;
extern const char* DIR_TX;
extern const char* DIR_AUDIO_TX;

extern const char* LC_PATH;
extern const char* SHT_PATH;

extern const char* LC_TX_PATH;
extern const char* SHT_TX_PATH;

bool        Init_SD();
void        LoadCell_write(char* timestamp, float load);
void        SHT_write(char* timestamp, float temp, float humi);
const char* record_wav_file(const DateTime& now, const char* saveDir, bool writeTx = false);
void        write_wav_header(File file, uint32_t data_size);
void        clearTxFiles();
void        SD_Sleep();
uint8_t     readNightSleepFlag();
void        writeNightSleepFlag(uint8_t val);

#endif 
