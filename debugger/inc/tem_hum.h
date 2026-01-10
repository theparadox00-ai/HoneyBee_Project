#ifndef TEM_HUM_H
#define TEM_HUM_H

#include "config.h"

class T_S_sensor {
public:
    void init();
    void readTempHum(float &temp, float &hum);
private:
    Adafruit_SHT4x sht4;
};
#endif
