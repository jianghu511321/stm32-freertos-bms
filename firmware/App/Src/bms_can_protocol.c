#include "bms_can_protocol.h"

#include <string.h>

static void put_u16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)(value >> 8);
}

static uint16_t get_u16(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static void put_u32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)((value >> 8) & 0xFFU);
    dst[2] = (uint8_t)((value >> 16) & 0xFFU);
    dst[3] = (uint8_t)(value >> 24);
}

static void encode_cells(const BmsSnapshot *snapshot,
                         uint32_t first_cell,
                         uint16_t id,
                         BmsCanFrame *frame)
{
    uint32_t i;
    frame->id = id;
    frame->dlc = 8U;
    put_u16(&frame->data[0], snapshot->sample.sequence);
    for (i = 0U; i < 3U; ++i) {
        put_u16(&frame->data[2U + (2U * i)], snapshot->sample.cell_mv[first_cell + i]);
    }
}

size_t BmsCan_EncodeSnapshot(const BmsSnapshot *snapshot,
                             uint8_t dropped_frames,
                             BmsCanFrame frames[BMS_CAN_FRAME_COUNT])
{
    if ((snapshot == NULL) || (frames == NULL)) {
        return 0U;
    }

    memset(frames, 0, sizeof(BmsCanFrame) * BMS_CAN_FRAME_COUNT);
    encode_cells(snapshot, 0U, BMS_CAN_ID_CELL_1_3, &frames[0]);
    encode_cells(snapshot, 3U, BMS_CAN_ID_CELL_4_6, &frames[1]);
    encode_cells(snapshot, 6U, BMS_CAN_ID_CELL_7_9, &frames[2]);
    encode_cells(snapshot, 9U, BMS_CAN_ID_CELL_10_12, &frames[3]);

    frames[4].id = BMS_CAN_ID_PACK;
    frames[4].dlc = 8U;
    put_u16(&frames[4].data[0], snapshot->sample.sequence);
    put_u16(&frames[4].data[2], snapshot->pack_voltage_mv);
    put_u16(&frames[4].data[4], (uint16_t)snapshot->sample.pack_current_ca);
    put_u16(&frames[4].data[6], snapshot->fault_flags);

    frames[5].id = BMS_CAN_ID_TEMP_1_3;
    frames[5].dlc = 8U;
    put_u16(&frames[5].data[0], snapshot->sample.sequence);
    put_u16(&frames[5].data[2], (uint16_t)snapshot->sample.ntc_dc[0]);
    put_u16(&frames[5].data[4], (uint16_t)snapshot->sample.ntc_dc[1]);
    put_u16(&frames[5].data[6], (uint16_t)snapshot->sample.ntc_dc[2]);

    frames[6].id = BMS_CAN_ID_TEMP_4_LIMITS;
    frames[6].dlc = 8U;
    put_u16(&frames[6].data[0], snapshot->sample.sequence);
    put_u16(&frames[6].data[2], (uint16_t)snapshot->sample.ntc_dc[3]);
    put_u16(&frames[6].data[4], snapshot->min_cell_mv);
    put_u16(&frames[6].data[6], snapshot->max_cell_mv);

    frames[7].id = BMS_CAN_ID_HEALTH;
    frames[7].dlc = 8U;
    put_u16(&frames[7].data[0], snapshot->sample.sequence);
    put_u32(&frames[7].data[2], snapshot->sample.timestamp_ms);
    frames[7].data[6] = dropped_frames;
    frames[7].data[7] = (snapshot->fault_flags == 0U) ? 0U : 1U;

    return BMS_CAN_FRAME_COUNT;
}

bool BmsCan_DecodeFrame(const BmsCanFrame *frame, BmsSnapshot *snapshot)
{
    uint32_t i;
    uint32_t first_cell;

    if ((frame == NULL) || (snapshot == NULL) || (frame->dlc != 8U)) {
        return false;
    }
    snapshot->sample.sequence = get_u16(&frame->data[0]);

    if ((frame->id >= BMS_CAN_ID_CELL_1_3) &&
        (frame->id <= BMS_CAN_ID_CELL_10_12)) {
        first_cell = (uint32_t)(frame->id - BMS_CAN_ID_CELL_1_3) * 3U;
        for (i = 0U; i < 3U; ++i) {
            snapshot->sample.cell_mv[first_cell + i] = get_u16(&frame->data[2U + (2U * i)]);
        }
        return true;
    }

    switch (frame->id) {
    case BMS_CAN_ID_PACK:
        snapshot->pack_voltage_mv = get_u16(&frame->data[2]);
        snapshot->sample.pack_current_ca = (int16_t)get_u16(&frame->data[4]);
        snapshot->fault_flags = get_u16(&frame->data[6]);
        return true;
    case BMS_CAN_ID_TEMP_1_3:
        snapshot->sample.ntc_dc[0] = (int16_t)get_u16(&frame->data[2]);
        snapshot->sample.ntc_dc[1] = (int16_t)get_u16(&frame->data[4]);
        snapshot->sample.ntc_dc[2] = (int16_t)get_u16(&frame->data[6]);
        return true;
    case BMS_CAN_ID_TEMP_4_LIMITS:
        snapshot->sample.ntc_dc[3] = (int16_t)get_u16(&frame->data[2]);
        snapshot->min_cell_mv = get_u16(&frame->data[4]);
        snapshot->max_cell_mv = get_u16(&frame->data[6]);
        return true;
    case BMS_CAN_ID_HEALTH:
        snapshot->sample.timestamp_ms = (uint32_t)frame->data[2] |
            ((uint32_t)frame->data[3] << 8) |
            ((uint32_t)frame->data[4] << 16) |
            ((uint32_t)frame->data[5] << 24);
        return true;
    default:
        return false;
    }
}

