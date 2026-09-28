#ifndef BMS_SENSOR_SIM_H
#define BMS_SENSOR_SIM_H

#include "bms_types.h"

void BmsSensorSim_Init(void);
void BmsSensorSim_Read(uint32_t timestamp_ms, BmsSample *sample);

#endif

