#ifndef BMS_CAN_TRANSPORT_H
#define BMS_CAN_TRANSPORT_H

#include <stdbool.h>

#include "bms_types.h"

bool BmsCanTransport_Init(void);
bool BmsCanTransport_Send(const BmsCanFrame *frame);
void BmsCanTransport_OnRxPendingFromISR(void);
void BmsCanTransport_OnErrorFromISR(uint32_t error_code);

#endif

