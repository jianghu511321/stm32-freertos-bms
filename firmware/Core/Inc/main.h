#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

#ifndef BMS_CPU_SIMULATOR
#define BMS_CPU_SIMULATOR 1
#endif

extern CAN_HandleTypeDef hcan;
extern TIM_HandleTypeDef htim4;

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif

