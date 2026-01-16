/*  SPI Pre configured SPI (SD) - 18 (SCK/SCLK) , 19 (MISO) , 23 (MOSI) , 13 (CS) -------------- Power : 5v
    I2C (Rtc) - 17 (SDA) , 16 (SCL) , 33 (SQW) -------------- Power : 3.3v
    I2C (SHT45) - 21 (SDA) , 22 (SCL) ----------- Power : 3.3v
    LoadCell - 5 (SCK) , 4 (DOUT) ------------- Power : 3.3v
    I2S (ICS-43434) - 25 (SCK) , 26 (WS) , 27 (SD) ------------ Power : 3.3v
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "FS.h"
#include "SD.h"

// Sensors & Modules
#include <HX711_ADC.h>    // Weight sensing for bee hive 
#include <RTClib.h>       // DS3231 Timekeeping 
#include "Adafruit_SHT4x.h" // Precision Temp/Humidity 
#include <ESP_Mail_Client.h> // Email alerts for swarming 
#include <MAX1704X.h>


// ESP32 Pins Configuration
#define SD_CS_PIN 13    // Standard CS 
#define HX711_DOUT_PIN 4    // Example GPIO for HX711 
#define HX711_SCK_PIN  5    // Example GPIO for HX711
#define CLOCK_INTERRUPT_PIN 33 // Clock Sleep Interrupt

//AUDIO (ICS43434)
#define I2S_SCK_PIN 25      // Serial Clock 
#define I2S_WS_PIN  26      // Word Select 
#define I2S_SD_PIN  27      // Serial Data 
#define I2S_PORT I2S_NUM_0

// System Constants
#define CALIBRATION_FACTOR 404.70 // Replace with your field-tested value [
#define SLEEP_DURATION 10     // Logging interval for bee behavior

// Audio Settings
#define SAMPLE_RATE 16000      
#define RECORD_TIME_SEC 5     

// NTP server - Login 
extern const char* ntpServer ;
extern const long  gmtOffset_sec ; 
extern const int   daylightOffset_sec ;

// Wifi Pass 
extern const char* WIFI_SSID ;
extern const char* WIFI_PASS ;

// SMTP - Data transfer
extern const char* SMTP_HOST ;
extern const int   SMTP_PORT ;
extern const char* AUTHOR_EMAIL ;
extern const char* AUTHOR_PASSWORD ; 
extern const char* RECIPIENT_EMAIL ;

#endif











