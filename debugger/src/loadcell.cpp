#include "loadcell.h"

HX711_ADC LoadCell(HX711_DOUT_PIN, HX711_SCK_PIN);

float LoadCellReading(int bootCount, long &savedOffset) {
    LoadCell.begin();

    bool _tare = (bootCount == 0);

    LoadCell.start(STABILIZING_TIME, _tare);
    LoadCell.setCalFactor(CALIBRATION_FACTOR); 

    if (_tare) {
        savedOffset = LoadCell.getTareOffset();
        Serial.print("New Zero Point Saved: ");
        Serial.println(savedOffset);
    } else {
        LoadCell.setTareOffset(savedOffset);
        Serial.print("Zero Point Restored: ");
        Serial.println(savedOffset);
    }

    float weight = LoadCell.getData();

    digitalWrite(HX711_SCK_PIN, LOW);
    digitalWrite(HX711_SCK_PIN, HIGH);
    
    return weight;
}
