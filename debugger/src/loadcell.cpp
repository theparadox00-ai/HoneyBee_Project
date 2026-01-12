#include "loadcell.h"

HX711_ADC LoadCell(HX711_DOUT_PIN, HX711_SCK_PIN);

float LoadCellReading(int bootCount, long &savedOffset) {
    LoadCell.begin();

    bool _tare = (bootCount == 0);
    unsigned long stabilizingTime = 2000;

    LoadCell.start(stabilizingTime, _tare);
    LoadCell.setCalFactor("xxxx")); // get the value after calibration ..

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
