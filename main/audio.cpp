#include "audio.h"
#include "config.h"

i2s_chan_handle_t rx_chan = NULL;

static int32_t s_dma_buf[I2S_BUFFER_SAMPLES];

#define FLUSH_SAMPLES  (2048)

void init_i2s_mic() {
    Serial.println("[I2S] Initialising ICS43434 (24-bit mono)...");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO,I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = 8;    
    chan_cfg.dma_frame_num = 256;  

    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &rx_chan);
    if (err != ESP_OK) {
        Serial.printf("[I2S] ERR: i2s_new_channel: %s\n", esp_err_to_name(err));
        return;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_32BIT,
                        I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_BCLK,
            .ws   = (gpio_num_t)I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)I2S_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    err = i2s_channel_init_std_mode(rx_chan, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[I2S] ERR: init_std_mode: %s\n", esp_err_to_name(err));
        return;
    }

    err = i2s_channel_enable(rx_chan);
    if (err != ESP_OK) {
        Serial.printf("[I2S] ERR: channel_enable: %s\n", esp_err_to_name(err));
        return;
    }

    Serial.println("[I2S] Flushing startup buffer...");
    int32_t flush_buf[I2S_BUFFER_SAMPLES];
    int remaining = FLUSH_SAMPLES;
    while (remaining > 0) {
        int chunk = (remaining > I2S_BUFFER_SAMPLES) ? I2S_BUFFER_SAMPLES : remaining;
        size_t bytes_to_read = (size_t)chunk * sizeof(int32_t);
        size_t bytes_read    = 0;
        i2s_channel_read(rx_chan, flush_buf, bytes_to_read, &bytes_read, pdMS_TO_TICKS(200));
        int got = (int)(bytes_read / sizeof(int32_t));
        remaining -= (got > 0) ? got : chunk;  
    }

    Serial.println("[I2S] Microphone ready (16 kHz, 24-bit, mono).");
}

int read_i2s_data(int32_t* buffer, size_t num_samples) {
    if (!rx_chan || !buffer || num_samples == 0) return 0;
    if (num_samples > I2S_BUFFER_SAMPLES) num_samples = I2S_BUFFER_SAMPLES;

    size_t collected = 0;
    while (collected < num_samples) {
        size_t want       = num_samples - collected;
        size_t bytes_want = want * sizeof(int32_t);
        size_t bytes_got  = 0;

        esp_err_t err = i2s_channel_read(rx_chan,
                                         s_dma_buf,
                                         bytes_want,
                                         &bytes_got,
                                         portMAX_DELAY);
        if (err != ESP_OK) {
            Serial.printf("[I2S] ERR: channel_read: %s\n", esp_err_to_name(err));
            break;
        }

        size_t got = bytes_got / sizeof(int32_t);
        if (got == 0) continue;   
        for (size_t i = 0; i < got; i++) {
            buffer[collected + i] = s_dma_buf[i] >> 8;
        }
        collected += got;
    }

    return (int)collected;
}

void deinit_i2s_mic() {
    if (rx_chan) {
        i2s_channel_disable(rx_chan);
        i2s_del_channel(rx_chan);
        rx_chan = NULL;
        Serial.println("[I2S] Microphone de-initialised.");
    }
}
