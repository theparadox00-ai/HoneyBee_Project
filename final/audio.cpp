#include "audio.h"
#include "config.h"

i2s_chan_handle_t rx_chan;

void init_i2s_mic() {
    Serial.println("[I2S] Initializing ICS43434 microphone...");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = 8;
    chan_cfg.dma_frame_num = 512;

    i2s_new_channel(&chan_cfg, NULL, &rx_chan);

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        // ICS43434 outputs 24-bit data in a 32-bit slot – Philips/I2S standard mode
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
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

    i2s_channel_init_std_mode(rx_chan, &std_cfg);
    i2s_channel_enable(rx_chan);
    Serial.println("[I2S] Microphone ready.");
}

void read_i2s_data(int16_t* buffer, size_t num_samples) {
    // ICS43434 sends 24-bit audio packed into 32-bit words
    // We read stereo and take only the left channel
    size_t   bytes_to_read = num_samples * 2 * sizeof(int32_t);
    int32_t* raw_samples   = (int32_t*)malloc(bytes_to_read);

    if (!raw_samples) {
        Serial.println("[I2S] ERR: malloc failed in read_i2s_data");
        return;
    }

    size_t bytes_read = 0;
    i2s_channel_read(rx_chan, raw_samples, bytes_to_read, &bytes_read, portMAX_DELAY);

    size_t samples_read = bytes_read / sizeof(int32_t);
    size_t mono_index   = 0;

    for (size_t i = 0; i < samples_read; i += 2) {   // step 2: left ch only
        int32_t sample = raw_samples[i] >> 8;          // 32→24 bit
        // Clamp to int16 range
        if (sample >  32767) sample =  32767;
        if (sample < -32768) sample = -32768;
        if (mono_index < num_samples) {
            buffer[mono_index++] = (int16_t)sample;
        }
    }

    free(raw_samples);
}

void deinit_i2s_mic() {
    if (rx_chan) {
        i2s_channel_disable(rx_chan);
        i2s_del_channel(rx_chan);
        rx_chan = NULL;
        Serial.println("[I2S] Microphone de-initialised.");
    }
}
