#ifndef TRANSMISSION_H
#define TRANSMISSION_H

#include "config.h"
#include "sd_card.h"

bool Send_All_Data_Email(int soc, float voltage);

bool Send_NightSleep_Email(int soc, float voltage);

bool Send_LowBattery_Email(int soc, float voltage);

bool Send_WakeupEmail(int soc, float voltage);

#endif 
