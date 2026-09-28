#include "bms_sensor_sim.h"

#include <stddef.h>

static uint16_t sequence;

static int32_t triangle(uint32_t x, uint32_t period, int32_t amplitude)
{
    const uint32_t half = period / 2U;
    const uint32_t phase = x % period;
    const int32_t ramp = (phase < half) ? (int32_t)phase : (int32_t)(period - phase);
    return ((2 * amplitude * ramp) / (int32_t)half) - amplitude;
}

void BmsSensorSim_Init(void)
{
    sequence = 0U;
}

void BmsSensorSim_Read(uint32_t timestamp_ms, BmsSample *sample)
{
    uint32_t i;
    const uint32_t fault_phase = timestamp_ms % BMS_SIM_FAULT_CYCLE_MS;

    if (sample == NULL) {
        return;
    }

    sample->sequence = sequence++;
    sample->timestamp_ms = timestamp_ms;
    for (i = 0U; i < BMS_CELL_COUNT; ++i) {
        const int32_t ripple = triangle(timestamp_ms + (i * 137U), 4000U, 18);
        sample->cell_mv[i] = (uint16_t)(3650 + (int32_t)(i * 3U) + ripple);
    }
    sample->pack_current_ca = (int16_t)triangle(timestamp_ms, 6000U, 2500);
    for (i = 0U; i < BMS_NTC_COUNT; ++i) {
        sample->ntc_dc[i] = (int16_t)(250 + triangle(timestamp_ms + i * 419U, 8000U, 20));
    }

    /* Deterministic injections make the protection path visible in a debugger:
     * 10-12 s overvoltage, 20-22 s overtemperature in each 30 s cycle. */
    if ((fault_phase >= 10000U) && (fault_phase < 12000U)) {
        sample->cell_mv[4] = 4300U;
    }
    if ((fault_phase >= 20000U) && (fault_phase < 22000U)) {
        sample->ntc_dc[1] = 650;
    }
}

