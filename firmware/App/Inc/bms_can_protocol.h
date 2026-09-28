#ifndef BMS_CAN_PROTOCOL_H
#define BMS_CAN_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>

#include "bms_types.h"

size_t BmsCan_EncodeSnapshot(const BmsSnapshot *snapshot,
                             uint8_t dropped_frames,
                             BmsCanFrame frames[BMS_CAN_FRAME_COUNT]);
bool BmsCan_DecodeFrame(const BmsCanFrame *frame, BmsSnapshot *snapshot);

#endif

