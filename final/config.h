#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include <driver/i2s_std.h>

#include <HX711_ADC.h>
#include <RTClib.h>
#include <WiFi.h>
#include <SensirionI2cSht4x.h>
#include <ESP_Mail_Client.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

// ── Beehive ESP32 Pins ───────────────────────────────────────
#define SD_CS_PIN           13
#define HX711_DOUT_PIN       3 // D2 
#define HX711_SCK_PIN       38 // D3
#define CLOCK_INTERRUPT_PIN 14
#define MOSFET_PIN          21

// ── ICS43434 I2S Microphone Pins ─────────────────────────────
#define I2S_BCLK             4   // Bit Clock
#define I2S_WS               5   // Word Select (L/R Clock)
#define I2S_DIN              6   // Data In

// ── System Constants ─────────────────────────────────────────
#define CALIBRATION_FACTOR  404.70
#define SLEEP_DURATION      30     // seconds between wakeups

// ── WiFi Credentials ─────────────────────────────────────────
#define WIFI_SSID  "moto g14"
#define WIFI_PASS  "suryadiya04"

// ── Email Credentials ────────────────────────────────────────
#define SMTP_HOST        "smtp.gmail.com"
#define SMTP_PORT        465
#define AUTHOR_EMAIL     "surydiya04@gmail.com"
#define AUTHOR_PASSWORD  "yxjo gelb vyha zmep"
#define RECIPIENT_EMAIL  "surydiya04@gmail.com"

// ── Task Schedule (every N boots) ────────────────────────────
#define LC_INTERVAL      2    // load cell every 2 boots
#define SHT_INTERVAL     2    // SHT45 every 2 boots
#define EMAIL_INTERVAL   3    // email every 3 boots
#define AUDIO_INTERVAL   1    // record audio every boot

// ── Audio / WAV Parameters ───────────────────────────────────
#define SAMPLE_RATE         16000
#define NUM_CHANNELS        1
#define BITS_PER_SAMPLE     16
#define BYTES_PER_SAMPLE    2
#define RECORD_TIME         10              // seconds per recording
#define BYTE_RATE           (SAMPLE_RATE * NUM_CHANNELS * (BITS_PER_SAMPLE / 8))
#define WAV_DATA_SIZE       (RECORD_TIME * SAMPLE_RATE * (BITS_PER_SAMPLE / 8))
#define WAV_FILE_SIZE       (WAV_DATA_SIZE + 44)
#define BIT_SHIFT_AMOUNT    11
#define I2S_BUFFER_SAMPLES  512

#endif
