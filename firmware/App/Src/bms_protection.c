#include "bms_protection.h"

#include <stddef.h>
#include <string.h>

enum {
    COUNTER_OV = 0,
    COUNTER_UV,
    COUNTER_OT,
    COUNTER_CHARGE_OC,
    COUNTER_DISCHARGE_OC,
    COUNTER_COUNT
};

static bool any_cell_above(const BmsSample *sample, uint16_t threshold)
{
    uint32_t i;
    for (i = 0U; i < BMS_CELL_COUNT; ++i) {
        if (sample->cell_mv[i] >= threshold) {
            return true;
        }
    }
    return false;
}

static bool all_cells_below(const BmsSample *sample, uint16_t threshold)
{
    uint32_t i;
    for (i = 0U; i < BMS_CELL_COUNT; ++i) {
        if (sample->cell_mv[i] > threshold) {
            return false;
        }
    }
    return true;
}

static bool any_cell_below(const BmsSample *sample, uint16_t threshold)
{
    uint32_t i;
    for (i = 0U; i < BMS_CELL_COUNT; ++i) {
        if (sample->cell_mv[i] <= threshold) {
            return true;
        }
    }
    return false;
}

static bool all_cells_above(const BmsSample *sample, uint16_t threshold)
{
    uint32_t i;
    for (i = 0U; i < BMS_CELL_COUNT; ++i) {
        if (sample->cell_mv[i] < threshold) {
            return false;
        }
    }
    return true;
}

static bool any_temp_above(const BmsSample *sample, int16_t threshold)
{
    uint32_t i;
    for (i = 0U; i < BMS_NTC_COUNT; ++i) {
        if (sample->ntc_dc[i] >= threshold) {
            return true;
        }
    }
    return false;
}

static bool all_temps_below(const BmsSample *sample, int16_t threshold)
{
    uint32_t i;
    for (i = 0U; i < BMS_NTC_COUNT; ++i) {
        if (sample->ntc_dc[i] > threshold) {
            return false;
        }
    }
    return true;
}

static void debounce(BmsProtection *p,
                     uint32_t index,
                     uint16_t flag,
                     bool trip_condition,
                     bool release_condition)
{
    if ((p->active_flags & flag) == 0U) {
        p->release_count[index] = 0U;
        if (trip_condition) {
            if (p->trip_count[index] < (uint8_t)BMS_FAULT_TRIP_SAMPLES) {
                ++p->trip_count[index];
            }
            if (p->trip_count[index] >= BMS_FAULT_TRIP_SAMPLES) {
                p->active_flags |= flag;
                p->trip_count[index] = 0U;
            }
        } else {
            p->trip_count[index] = 0U;
        }
    } else {
        p->trip_count[index] = 0U;
        if (release_condition) {
            if (p->release_count[index] < (uint8_t)BMS_FAULT_RELEASE_SAMPLES) {
                ++p->release_count[index];
            }
            if (p->release_count[index] >= BMS_FAULT_RELEASE_SAMPLES) {
                p->active_flags &= (uint16_t)~flag;
                p->release_count[index] = 0U;
            }
        } else {
            p->release_count[index] = 0U;
        }
    }
}

void BmsProtection_Init(BmsProtection *protection)
{
    if (protection != NULL) {
        memset(protection, 0, sizeof(*protection));
    }
}

uint16_t BmsProtection_Evaluate(BmsProtection *p, const BmsSample *sample)
{
    if ((p == NULL) || (sample == NULL)) {
        return BMS_FAULT_NONE;
    }

    debounce(p, COUNTER_OV, BMS_FAULT_CELL_OVERVOLTAGE,
             any_cell_above(sample, BMS_CELL_OV_TRIP_MV),
             all_cells_below(sample, BMS_CELL_OV_RELEASE_MV));
    debounce(p, COUNTER_UV, BMS_FAULT_CELL_UNDERVOLTAGE,
             any_cell_below(sample, BMS_CELL_UV_TRIP_MV),
             all_cells_above(sample, BMS_CELL_UV_RELEASE_MV));
    debounce(p, COUNTER_OT, BMS_FAULT_OVERTEMPERATURE,
             any_temp_above(sample, BMS_TEMP_OT_TRIP_DC),
             all_temps_below(sample, BMS_TEMP_OT_RELEASE_DC));
    debounce(p, COUNTER_CHARGE_OC, BMS_FAULT_CHARGE_OVERCURRENT,
             sample->pack_current_ca >= BMS_CHARGE_OC_TRIP_CA,
             sample->pack_current_ca <= BMS_CHARGE_OC_RELEASE_CA);
    debounce(p, COUNTER_DISCHARGE_OC, BMS_FAULT_DISCHARGE_OVERCURRENT,
             sample->pack_current_ca <= BMS_DISCHARGE_OC_TRIP_CA,
             sample->pack_current_ca >= BMS_DISCHARGE_OC_RELEASE_CA);

    return p->active_flags;
}

void BmsSnapshot_Build(const BmsSample *sample,
                       uint16_t fault_flags,
                       BmsSnapshot *snapshot)
{
    uint32_t i;
    uint32_t pack_mv = 0U;

    if ((sample == NULL) || (snapshot == NULL)) {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->sample = *sample;
    snapshot->min_cell_mv = sample->cell_mv[0];
    snapshot->max_cell_mv = sample->cell_mv[0];
    for (i = 0U; i < BMS_CELL_COUNT; ++i) {
        const uint16_t mv = sample->cell_mv[i];
        pack_mv += mv;
        if (mv < snapshot->min_cell_mv) {
            snapshot->min_cell_mv = mv;
        }
        if (mv > snapshot->max_cell_mv) {
            snapshot->max_cell_mv = mv;
        }
    }
    snapshot->pack_voltage_mv = (pack_mv > UINT16_MAX) ? UINT16_MAX : (uint16_t)pack_mv;
    snapshot->fault_flags = fault_flags;
}
