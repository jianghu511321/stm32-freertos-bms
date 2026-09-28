#ifndef BMS_TYPES_H
#define BMS_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "bms_config.h"

typedef enum {
    BMS_FAULT_NONE             = 0U,
    BMS_FAULT_CELL_OVERVOLTAGE = 1U << 0,
    BMS_FAULT_CELL_UNDERVOLTAGE= 1U << 1,
    BMS_FAULT_OVERTEMPERATURE  = 1U << 2,
    BMS_FAULT_CHARGE_OVERCURRENT = 1U << 3,
    BMS_FAULT_DISCHARGE_OVERCURRENT = 1U << 4,
    BMS_FAULT_CAN              = 1U << 5
} BmsFault;

typedef struct {
    uint16_t sequence;
    uint32_t timestamp_ms;
    uint16_t cell_mv[BMS_CELL_COUNT];
    int16_t pack_current_ca;
    int16_t ntc_dc[BMS_NTC_COUNT];
} BmsSample;

typedef struct {
    BmsSample sample;
    uint16_t pack_voltage_mv;
    uint16_t min_cell_mv;
    uint16_t max_cell_mv;
    uint16_t fault_flags;
} BmsSnapshot;

typedef struct {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];
} BmsCanFrame;

#endif

