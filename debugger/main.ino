/* This is the main program which represents the workflow of the whole program for data logging 
   While deploying the system wakes up every 10s for getting the Audio data , every 20s for getting the load cell sensor reading (Storing it in SD card)
   every 15 mins for getting the SHT45 sensor reading 
   The data is then sent to SMTP server to the Gmail
*/

#include "config.h"
#include "loadcell.h"
#include "tem_hum.h"
#include "rtc_config.h" 
#include "sd_data.h"
#include "transmission.h"
#include "audio.h"
#include "battery_status.h"

RTC_DATA_ATTR int bootCount = 0; 
RTC_DATA_ATTR long rtc_tare_offset = 0;

const char* WIFI_SSID = "vivo V23e 5G";
const char* WIFI_PASS = "suryadiya";

T_S_sensor SHT45;

void setup() {
    Serial.begin(115200);
    delay(1000); 

    if (!initRTC()) {
        Serial.println("RTC Init Failed!");
    }

    if (!Init_SD()) {
        Serial.println("SD Init Failed!");
    }

    char timeStr[25];
    if(bootCount != 0){
        Time_(timeStr);
        Serial.println(timeStr);
    }

    if(bootCount == 0){
        Serial.println("Reset Calibration !!!! ");
        float volt,per = 0;
        clock_Synchronization();
        initBatteryGauge(bootCount);
        BatteryStatus(volt, per);
        Serial.print("Battery Voltage : ");
        Serial.println(volt);
        Serial.print("Battery Percentage : ");
        Serial.println(per);
    }

    Serial.print("Boot Count: ");
    Serial.println(bootCount);

    Serial.println("--- Starting Audio ---");
    audio_init();
    audio_record(String(timeStr)); 
    audio_sleep(); 
    Serial.println("--- Audio Done ---");

    if((bootCount % 2 == 0) && (bootCount != 0)){
         float loadValue = LoadCellReading(bootCount,rtc_tare_offset);
         if (loadValue != -999.0) {
         LoadCell_write(timeStr, loadValue);
         }
         Serial.print("Weight : ");
         Serial.println(loadValue);
   }
   
    if ((bootCount % 900 == 0) && (bootCount != 0)) {
        SHT45.init();
        float tempValue = -999.0;
        float humiValue = -999.0;
        SHT45.readTempHum(tempValue, humiValue);
        Serial.print(tempValue);
        Serial.print(",");
        Serial.println(humiValue);
        if ((tempValue != -999.0) && (humiValue != -999.0)) {
            SHT_write(timeStr, tempValue, humiValue);
            Serial.println("SHT Data Logged (Even Boot)");
        }
   }

   if ((bootCount % 10800 == 0) && (bootCount != 0) ) {
         float volt , per = 0;
         WiFi.persistent(false);
         WiFi.mode(WIFI_STA);
         delay(100);
         initBatteryGauge(bootCount);
         BatteryStatus(volt,per);
         Serial.print("Battery Voltage : ");
         Serial.println(volt);
         Serial.print("Battery Percentage : ");
         Serial.println(per);
         WiFi.begin(WIFI_SSID, WIFI_PASS);
         if (clock_Synchronization()) {
            Serial.println("RTC Synchronized with NTP.");
            Serial.println(timeStr);
         } else {
             Serial.println("Clock Sync Failed.");
         }
         unsigned long startAttempt = millis();
         while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 25000) {
            Serial.print(".");
         }
         if (WiFi.status() == WL_CONNECTED) {
            if (clock_Synchronization()) {
                Serial.println("RTC Synchronized with NTP.");
                Serial.println(timeStr);
            } else {
                Serial.println("Clock Sync Failed.");
            }
            if (Send_All_Data_Email(volt,per)) {
                Serial.println("Email Sent Successfully.");
            } else {
                Serial.println("Email Failed.");
            }
        } else {
            Serial.printf("WiFi Failed. Status: %d\n", WiFi.status());
        }

        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }

   if(bootCount > 86400){
      bootCount = 0;
   }
   
   SD_Sleep(); 
   Alarm_();   
   bootCount++;
   Serial.println("Entering deep sleep...");
   Serial.flush();
   esp_deep_sleep_start();
}


void loop(){}
