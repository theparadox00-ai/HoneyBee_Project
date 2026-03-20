#ifndef FILE_H
#define FILE_H

#include "config.h"
#include "sd_card.h"

size_t BootCount(const char* idxDir, const char* idxFile);
void   writeBootCount(const char* idxDir, const char* idxFile, size_t count);

#endif
