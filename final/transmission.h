#ifndef TRANSMISSION_H
#define TRANSMISSION_H

#include "config.h"
#include "sd_card.h"

// Sends TX buffer CSVs + WAV files as email attachments.
// Clears TX buffers only on confirmed success.
bool Send_All_Data_Email(int soc, float voltage);

#endif
