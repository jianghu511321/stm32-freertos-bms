#ifndef BMS_CONFIG_H
#define BMS_CONFIG_H

#include <stdint.h>

#define BMS_CELL_COUNT                 12U
#define BMS_NTC_COUNT                  4U
#define BMS_SAMPLE_PERIOD_MS           10U

#define BMS_CELL_OV_TRIP_MV            4200U
#define BMS_CELL_OV_RELEASE_MV         4100U
#define BMS_CELL_UV_TRIP_MV            2800U
#define BMS_CELL_UV_RELEASE_MV         3000U
#define BMS_TEMP_OT_TRIP_DC            600
#define BMS_TEMP_OT_RELEASE_DC         550
#define BMS_CHARGE_OC_TRIP_CA          10000
#define BMS_CHARGE_OC_RELEASE_CA       9500
#define BMS_DISCHARGE_OC_TRIP_CA       (-15000)
#define BMS_DISCHARGE_OC_RELEASE_CA    (-14000)

#define BMS_FAULT_TRIP_SAMPLES         3U
#define BMS_FAULT_RELEASE_SAMPLES      50U
#define BMS_FILTER_SHIFT               3U

#define BMS_CAN_ID_CELL_1_3            0x300U
#define BMS_CAN_ID_CELL_4_6            0x301U
#define BMS_CAN_ID_CELL_7_9            0x302U
#define BMS_CAN_ID_CELL_10_12          0x303U
#define BMS_CAN_ID_PACK                0x304U
#define BMS_CAN_ID_TEMP_1_3            0x305U
#define BMS_CAN_ID_TEMP_4_LIMITS       0x306U
#define BMS_CAN_ID_HEALTH              0x307U
#define BMS_CAN_FRAME_COUNT            8U

#define BMS_SIM_FAULT_CYCLE_MS         30000U

/* Keil's CPU simulator does not model bxCAN reliably. Keep this enabled for
 * software loopback. Set to 0 only when running on a board with a CAN PHY. */
#ifndef BMS_TRANSPORT_SOFTWARE_LOOPBACK
#define BMS_TRANSPORT_SOFTWARE_LOOPBACK 1
#endif

#endif

