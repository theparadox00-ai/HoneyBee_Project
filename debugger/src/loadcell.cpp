#include "loadcell.h"

HX711_ADC LoadCell(HX711_DOUT_PIN, HX711_SCK_PIN);

void lc_init() {
    LoadCell.begin();
    LoadCell.start(10); // experimental value to improve the latency 
    LoadCell.setCalFactor(CALIBRATION_FACTOR); 
}

float LoadCellReading() {
    unsigned long timeout = millis() + 10; // experimental value to improve teh latency
    while (millis() < timeout) {
        if (LoadCell.update()) {
            return LoadCell.getData();
        }
    }
    return -999.0;
}

void lc_sleep() {
    LoadCell.powerDown();
}
