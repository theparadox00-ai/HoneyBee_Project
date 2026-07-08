#ifndef LOADCELL_H
#define LOADCELL_H

#include "config.h"

void LoadCell_Init(long& savedOffset, bool offsetReady);

bool LoadCell_ReadOffsetFromSD(long* out);

float LoadCell_Read();

void LoadCell_Sleep();

#endif 
