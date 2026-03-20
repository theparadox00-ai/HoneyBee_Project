#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include "driver/i2s_std.h"

extern i2s_chan_handle_t rx_chan;

void init_i2s_mic();
void read_i2s_data(int16_t* buffer, size_t num_samples);
void deinit_i2s_mic();

#endif
