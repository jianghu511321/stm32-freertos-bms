#ifndef BMS_PROTECTION_H
#define BMS_PROTECTION_H

#include "bms_types.h"

typedef struct {
    uint16_t active_flags;
    uint8_t trip_count[5];
    uint8_t release_count[5];
} BmsProtection;

void BmsProtection_Init(BmsProtection *protection);
uint16_t BmsProtection_Evaluate(BmsProtection *protection,
                                const BmsSample *sample);
void BmsSnapshot_Build(const BmsSample *sample,
                       uint16_t fault_flags,
                       BmsSnapshot *snapshot);

#endif

