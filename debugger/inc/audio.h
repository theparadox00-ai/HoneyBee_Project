#ifndef AUDIO_LOGGER_H
#define AUDIO_LOGGER_H

#include "config.h"

void audio_init();
void audio_record(String timestamp);
void audio_sleep();

#endif
