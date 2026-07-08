#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include <driver/i2s_std.h>

#include <HX711.h>
#include <RTClib.h>
#include <WiFi.h>
#include <SensirionI2cSht4x.h>
#include <ESP_Mail_Client.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

#define SD_CS_PIN           13   // D10
#define SD_SCK_PIN          17
#define SD_MISO_PIN         16
#define SD_MOSI_PIN         15

#define HX711_DOUT_PIN       3   // D2
#define HX711_SCK_PIN       38   // D3
#define MOSFET_PIN          21   // Power switch for peripherals // D13

// ── ICS43434 I²S Microphone Pins ─────────────────────────────────────────────
#define I2S_BCLK             4   // Bit Clock // A0
#define I2S_WS               5   // Word Select / LRCK // A1
#define I2S_DIN              6   // Data In // A2

// ── Load Cell ─────────────────────────────────────────────────────────────────
#define CALIBRATION_FACTOR  260.03
#define OFFSET_FILE         "/LoadCell/offset.txt"   // persisted tare offset

// ── Deep-Sleep / Alarm ───────────────────────────────────────────────────────
#define SLEEP_DURATION      30   // seconds between wakeups (daytime cycle)

// ── WiFi Credentials ─────────────────────────────────────────────────────────
#define WIFI_SSID  "vivo Y16"
#define WIFI_PASS  "0987654321"

// ── Email Credentials ────────────────────────────────────────────────────────
#define SMTP_HOST        "smtp.gmail.com"
#define SMTP_PORT        465
#define AUTHOR_EMAIL     "suryadiya04@gmail.com"
#define AUTHOR_PASSWORD  "vurw zeod fbkm xkvn"
#define RECIPIENT_EMAIL  "suryadiya04@gmail.com"

// ── Task Schedule (every N boots) ────────────────────────────────────────────
#define LC_INTERVAL      1    // load cell every 2 boots
#define SHT_INTERVAL     1   // SHT45 every 15 boots
#define EMAIL_INTERVAL   1 // email every 2000 boots
#define AUDIO_INTERVAL   1    // audio every boot

#define SAMPLE_RATE         16000
#define NUM_CHANNELS        1
#define BITS_PER_SAMPLE     24
#define BYTES_PER_SAMPLE    3
#define RECORD_TIME         10                // seconds per recording
#define BYTE_RATE           (SAMPLE_RATE * NUM_CHANNELS * BYTES_PER_SAMPLE)
#define WAV_DATA_SIZE       (RECORD_TIME * SAMPLE_RATE * BYTES_PER_SAMPLE)
#define WAV_FILE_SIZE       (WAV_DATA_SIZE + 44)
#define I2S_BUFFER_SAMPLES  512               // samples per DMA read chunk

#define LOCATION_GMT_OFFSET_SEC  19800
#define SCHEDULER_ACTIVE_ONLY    true

#endif 
