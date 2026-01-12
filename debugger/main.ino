/* This is the main program which represents the workflow of the whole program for data logging 
   While deploying the system the system wakes up every 20s for getting the load cell sensor reading (Storing it in SD card)
   every 15 mins for getting the SHT45 sensor reading 
   The data is then sent to SMTP server to the Gmail
*/

#include "config.h"
#include "loadcell.h"
#include "tem_hum.h"
#include "rtc_config.h" 
#include "sd_data.h"
#include "transmission.h"

RTC_DATA_ATTR int bootCount = 1; 

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
    lc_init(); 
    SHT45.init(); 

    char timeStr[25];
    if(bootCount != 1){
        Time_(timeStr);
        Serial.println(timeStr);
    }

    if(bootCount == 1){
        clock_Synchronization();
    }

    Serial.print("Boot Count: ");
    Serial.println(bootCount);

    float loadValue = LoadCellReading();
    if (loadValue != -999.0) {
        LoadCell_write(timeStr, loadValue);
    }
    Serial.print("Weight : ");
    Serial.println(loadValue);

    if (bootCount % 1 == 0) {
        float tempValue = -999.0;
        float humiValue = -999.0;
        SHT45.readTempHum(tempValue, humiValue);

        Serial.print(tempValue);
        Serial.print(",");
        Serial.println(humiValue);
        if (tempValue != -999.0) {
            SHT_write(timeStr, tempValue, humiValue);
            Serial.println("SHT Data Logged (Even Boot)");
        }
    }

    if ((bootCount % 6 == 0)) {
        if(bootCount == 0){
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            if (clock_Synchronization()) {
                Serial.println("RTC Synchronized with NTP.");
                Serial.println(timeStr);
            } else {
                Serial.println("Clock Sync Failed.");
            }
        }
        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            if (clock_Synchronization()) {
                Serial.println("RTC Synchronized with NTP.");
                Serial.println(timeStr);
            } else {
                Serial.println("Clock Sync Failed.");
            }
            if (Send_All_Data_Email()) {
                Serial.println("Email Sent Successfully.");
            } else {
                Serial.println("Email Failed.");
            }
        } else {
            Serial.println("WiFi Failed.");
        }
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }

    lc_sleep();
    SD_Sleep(); 
    Alarm_();   
    bootCount++;
    Serial.println("Entering deep sleep...");
    Serial.flush();
    esp_deep_sleep_start();
}


void loop(){}
