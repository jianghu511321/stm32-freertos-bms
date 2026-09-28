#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bms_can_protocol.h"
#include "bms_filter.h"
#include "bms_protection.h"
#include "bms_sensor_sim.h"

static void test_protocol_round_trip(void)
{
    BmsSample sample = {0};
    BmsSnapshot input;
    BmsSnapshot output = {0};
    BmsCanFrame frames[BMS_CAN_FRAME_COUNT];
    uint32_t i;

    sample.sequence = 0x1234U;
    sample.timestamp_ms = 0x10203040U;
    sample.pack_current_ca = -3210;
    for (i = 0U; i < BMS_CELL_COUNT; ++i) {
        sample.cell_mv[i] = (uint16_t)(3500U + i);
    }
    for (i = 0U; i < BMS_NTC_COUNT; ++i) {
        sample.ntc_dc[i] = (int16_t)(240 + (int16_t)i);
    }
    BmsSnapshot_Build(&sample, BMS_FAULT_CELL_UNDERVOLTAGE, &input);

    assert(BmsCan_EncodeSnapshot(&input, 2U, frames) == BMS_CAN_FRAME_COUNT);
    for (i = 0U; i < BMS_CAN_FRAME_COUNT; ++i) {
        assert(BmsCan_DecodeFrame(&frames[i], &output));
    }
    assert(memcmp(input.sample.cell_mv, output.sample.cell_mv,
                  sizeof(input.sample.cell_mv)) == 0);
    assert(memcmp(input.sample.ntc_dc, output.sample.ntc_dc,
                  sizeof(input.sample.ntc_dc)) == 0);
    assert(output.sample.sequence == input.sample.sequence);
    assert(output.sample.timestamp_ms == input.sample.timestamp_ms);
    assert(output.sample.pack_current_ca == input.sample.pack_current_ca);
    assert(output.pack_voltage_mv == input.pack_voltage_mv);
    assert(output.fault_flags == input.fault_flags);
}

static void test_protection_debounce_and_release(void)
{
    BmsProtection protection;
    BmsSample sample = {0};
    uint32_t i;

    BmsProtection_Init(&protection);
    for (i = 0U; i < BMS_CELL_COUNT; ++i) {
        sample.cell_mv[i] = 3600U;
    }
    for (i = 0U; i < BMS_NTC_COUNT; ++i) {
        sample.ntc_dc[i] = 250;
    }

    sample.cell_mv[2] = BMS_CELL_OV_TRIP_MV;
    for (i = 0U; i < BMS_FAULT_TRIP_SAMPLES - 1U; ++i) {
        assert(BmsProtection_Evaluate(&protection, &sample) == BMS_FAULT_NONE);
    }
    assert((BmsProtection_Evaluate(&protection, &sample) &
            BMS_FAULT_CELL_OVERVOLTAGE) != 0U);

    sample.cell_mv[2] = BMS_CELL_OV_RELEASE_MV;
    for (i = 0U; i < BMS_FAULT_RELEASE_SAMPLES - 1U; ++i) {
        assert((BmsProtection_Evaluate(&protection, &sample) &
                BMS_FAULT_CELL_OVERVOLTAGE) != 0U);
    }
    assert((BmsProtection_Evaluate(&protection, &sample) &
            BMS_FAULT_CELL_OVERVOLTAGE) == 0U);
}

static void test_simulator_injects_faults(void)
{
    BmsSample sample;
    BmsSensorSim_Init();
    BmsSensorSim_Read(10500U, &sample);
    assert(sample.cell_mv[4] == 4300U);
    BmsSensorSim_Read(20500U, &sample);
    assert(sample.ntc_dc[1] == 650);
}

int main(void)
{
    test_protocol_round_trip();
    test_protection_debounce_and_release();
    test_simulator_injects_faults();
    puts("all tests passed");
    return 0;
}

