#include "audio_logger.h"
#include <driver/i2s.h>
#include "FS.h"
#include "SD.h"

// Audio Settings
#define SAMPLE_RATE 16000      
#define RECORD_TIME_SEC 10     

// WAV Header Structure (Standard for PC Playback)
struct WavHeader {
  char riff_tag[4];
  int32_t riff_length;
  char wave_tag[4];
  char fmt_tag[4];
  int32_t fmt_length;
  int16_t audio_format;
  int16_t num_channels;
  int32_t sample_rate;
  int32_t byte_rate;
  int16_t block_align;
  int16_t bits_per_sample;
  char data_tag[4];
  int32_t data_length;
};

// Helper: Writes the metadata so the file plays on a computer
void writeWavHeader(File &file, int wavSize) {
    WavHeader header;
    memcpy(header.riff_tag, "RIFF", 4);
    header.riff_length = wavSize + 36;
    memcpy(header.wave_tag, "WAVE", 4);
    memcpy(header.fmt_tag, "fmt ", 4);
    header.fmt_length = 16;
    header.audio_format = 1;
    header.num_channels = 1;
    header.sample_rate = SAMPLE_RATE;
    header.byte_rate = SAMPLE_RATE * 2;
    header.block_align = 2;
    header.bits_per_sample = 16;
    memcpy(header.data_tag, "data", 4);
    header.data_length = wavSize;
    file.write((uint8_t*)&header, sizeof(WavHeader));
}

// ---------------------------------------------------------
// 1. INITIALISATION
// ---------------------------------------------------------
void audio_init() {
    Serial.println("[Audio] Init I2S...");

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    // Uses Pins defined in CONFIG.H
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN, // Pin 25
        .ws_io_num = I2S_WS_PIN,   // Pin 26
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_PIN  // Pin 27
    };

    if (i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) != ESP_OK) {
        Serial.println("[Audio] Driver Error");
    }
    if (i2s_set_pin(I2S_PORT, &pin_config) != ESP_OK) {
        Serial.println("[Audio] Pin Error");
    }
}

// ---------------------------------------------------------
// 2. DATA COLLECTION (Recording to SD)
// ---------------------------------------------------------
void audio_record(String timestamp) {
    // 1. Create Filename
    String path = "/" + timestamp + ".wav";
    path.replace(":", "-"); 
    
    Serial.print("[Audio] Recording: ");
    Serial.println(path);

    // 2. Open SD File
    File file = SD.open(path.c_str(), FILE_WRITE);
    if (!file) {
        Serial.println("[Audio] SD Error");
        return;
    }

    // 3. Allocate RAM (2KB Buffer)
    const int32_t i2s_buffer_len = 1024;
    int16_t *i2s_read_buff = (int16_t*)calloc(i2s_buffer_len, sizeof(int16_t));
    
    // 4. Record Loop
    file.seek(44); // Skip header space
    size_t bytes_read;
    uint32_t total_bytes = 0;
    uint32_t expected_bytes = SAMPLE_RATE * 2 * RECORD_TIME_SEC;
    
    while (total_bytes < expected_bytes) {
        // Read Mic (DMA) -> Buffer -> Write SD
        i2s_read(I2S_PORT, i2s_read_buff, i2s_buffer_len * 2, &bytes_read, portMAX_DELAY);
        file.write((uint8_t*)i2s_read_buff, bytes_read);
        total_bytes += bytes_read;
    }

    // 5. Finish File
    file.seek(0);
    writeWavHeader(file, total_bytes);
    file.close();
    free(i2s_read_buff);
    
    Serial.println("[Audio] Saved.");
}

// ---------------------------------------------------------
// 3. SLEEP
// ---------------------------------------------------------
void audio_sleep() {
    i2s_driver_uninstall(I2S_PORT);
    Serial.println("[Audio] OFF");
}
