#ifndef BMS_FREERTOS_APP_H
#define BMS_FREERTOS_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "bms_types.h"

extern QueueHandle_t g_bms_sample_queue;
extern QueueHandle_t g_bms_snapshot_queue;
extern QueueHandle_t g_can_rx_queue;
extern TaskHandle_t g_fault_task_handle;

void MX_FREERTOS_Init(void);
void BmsTasks_NotifyCanFaultFromISR(uint32_t error_code);

#ifdef __cplusplus
}
#endif

#endif

