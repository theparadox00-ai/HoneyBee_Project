#include "tem_hum.h"

void T_S_sensor::init() {
    if (!sht4.begin()) {
        Serial.println("SHT45 Not Found");
    }
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
}

void T_S_sensor::readTempHum(float &temp, float &hum) {
    sensors_event_t humidity, temp_event;
    sht4.getEvent(&humidity, &temp_event);
    temp = temp_event.temperature;
    hum = humidity.relative_humidity;
}
