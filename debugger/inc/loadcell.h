#ifndef LOADCELL_H
#define LOADCELL_H

#include "config.h"

#define HX711_DOUT_PIN  4  
#define HX711_SCK_PIN   5

float LoadCellReading(int bootCount, long &savedOffset); 

#endif
