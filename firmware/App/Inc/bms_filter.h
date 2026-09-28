#ifndef BMS_FILTER_H
#define BMS_FILTER_H

#include <stdbool.h>

#include "bms_types.h"

typedef struct {
    bool initialized;
    int32_t cell_q[BMS_CELL_COUNT];
    int32_t current_q;
    int32_t ntc_q[BMS_NTC_COUNT];
} BmsFilter;

void BmsFilter_Init(BmsFilter *filter);
void BmsFilter_Apply(BmsFilter *filter,
                     const BmsSample *raw,
                     BmsSample *filtered);

#endif

