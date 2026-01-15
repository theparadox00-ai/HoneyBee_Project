/*Battery should be fully charged and the code is to executed from which the first bootCount will result in a calibration */

#ifndef BATTERY_STATUS_H
#define BATTERY_STATUS_H

#include "MAX1704X.h"

MAX170X gauge;

void initBatteryGauge(bootCount);
void BatteryStatus(float &volt, float &per);

#endif
