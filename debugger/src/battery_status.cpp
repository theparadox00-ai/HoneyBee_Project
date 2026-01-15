#include "battery_health.h"

void initBatteryGauge(bootCount) {
  Wire.begin();  
  if (!gauge.begin()) {
    Serial.println("MAX1704X not found!");
    return;
  }
  Serial.println("Battery gauge initialized.");
  if(bootCount == 0){
    gauge.reset();      
    delay(250);
    gauge.quickStart(); 
    delay(125);
  }
}

void BatteryStatus(float &volt, float &per) {
  volt = gauge.voltage();
  per = gauge.percent();
  gauge.sleep();
}
