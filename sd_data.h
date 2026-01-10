#ifndef SD_DATA_H
#define SD_DATA_H

#include "config.h"

extern const char* DIR_LOADCELL;
extern const char* DIR_SHT;

bool Init_SD();
void LoadCell_write(char* time, float load);
void SHT_write(char* time, float temp, float humi);
void SD_Sleep();

#endif
