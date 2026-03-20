#ifndef TEMP_HUM_H
#define TEMP_HUM_H

#include "config.h"

class T_S_sensor {
public:
    void init();
    void readTempHum(float &temp, float &hum);
private:
    SensirionI2cSht4x sht4;
};

#endif
