#include "bms_filter.h"

#include <stddef.h>
#include <string.h>

static int32_t filter_step(int32_t state_q, int32_t input)
{
    const int32_t scale = 1L << BMS_FILTER_SHIFT;
    const int32_t target_q = input * scale;
    return state_q + ((target_q - state_q) / scale);
}

void BmsFilter_Init(BmsFilter *filter)
{
    if (filter != NULL) {
        memset(filter, 0, sizeof(*filter));
    }
}

void BmsFilter_Apply(BmsFilter *filter,
                     const BmsSample *raw,
                     BmsSample *filtered)
{
    uint32_t i;

    if ((filter == NULL) || (raw == NULL) || (filtered == NULL)) {
        return;
    }

    *filtered = *raw;
    if (!filter->initialized) {
        for (i = 0U; i < BMS_CELL_COUNT; ++i) {
            filter->cell_q[i] = (int32_t)raw->cell_mv[i] * (1L << BMS_FILTER_SHIFT);
        }
        filter->current_q = (int32_t)raw->pack_current_ca * (1L << BMS_FILTER_SHIFT);
        for (i = 0U; i < BMS_NTC_COUNT; ++i) {
            filter->ntc_q[i] = (int32_t)raw->ntc_dc[i] * (1L << BMS_FILTER_SHIFT);
        }
        filter->initialized = true;
    } else {
        for (i = 0U; i < BMS_CELL_COUNT; ++i) {
            filter->cell_q[i] = filter_step(filter->cell_q[i], raw->cell_mv[i]);
        }
        filter->current_q = filter_step(filter->current_q, raw->pack_current_ca);
        for (i = 0U; i < BMS_NTC_COUNT; ++i) {
            filter->ntc_q[i] = filter_step(filter->ntc_q[i], raw->ntc_dc[i]);
        }
    }

    for (i = 0U; i < BMS_CELL_COUNT; ++i) {
        filtered->cell_mv[i] = (uint16_t)(filter->cell_q[i] / (1L << BMS_FILTER_SHIFT));
    }
    filtered->pack_current_ca = (int16_t)(filter->current_q / (1L << BMS_FILTER_SHIFT));
    for (i = 0U; i < BMS_NTC_COUNT; ++i) {
        filtered->ntc_dc[i] = (int16_t)(filter->ntc_q[i] / (1L << BMS_FILTER_SHIFT));
    }
}
