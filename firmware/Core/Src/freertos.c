#include "bms_freertos.h"

#include <string.h>

#include "bms_can_protocol.h"
#include "bms_can_transport.h"
#include "bms_filter.h"
#include "bms_protection.h"
#include "bms_sensor_sim.h"
#include "cmsis_os.h"
#include "main.h"

QueueHandle_t g_bms_sample_queue;
QueueHandle_t g_bms_snapshot_queue;
QueueHandle_t g_can_rx_queue;
TaskHandle_t g_fault_task_handle;

static volatile uint16_t g_last_fault_flags;
static volatile uint32_t g_last_can_error;
static volatile uint32_t g_loopback_rx_count;
static volatile uint8_t g_dropped_frames;

static void StartDefaultTask(void const *argument);
static void BmsSamplingTask(void const *argument);
static void BmsProcessTask(void const *argument);
static void BmsCanTxTask(void const *argument);
static void FaultTask(void const *argument);

void MX_FREERTOS_Init(void)
{
    osThreadId handle;

    g_bms_sample_queue = xQueueCreate(4U, sizeof(BmsSample));
    g_bms_snapshot_queue = xQueueCreate(1U, sizeof(BmsSnapshot));
    g_can_rx_queue = xQueueCreate(16U, sizeof(BmsCanFrame));
    if ((g_bms_sample_queue == NULL) || (g_bms_snapshot_queue == NULL) ||
        (g_can_rx_queue == NULL)) {
        Error_Handler();
    }

    osThreadDef(defaultTask, StartDefaultTask, osPriorityLow, 0U, 128U);
    handle = osThreadCreate(osThread(defaultTask), NULL);
    if (handle == NULL) { Error_Handler(); }

    osThreadDef(sampleTask, BmsSamplingTask, osPriorityHigh, 0U, 192U);
    handle = osThreadCreate(osThread(sampleTask), NULL);
    if (handle == NULL) { Error_Handler(); }

    osThreadDef(processTask, BmsProcessTask, osPriorityAboveNormal, 0U, 256U);
    handle = osThreadCreate(osThread(processTask), NULL);
    if (handle == NULL) { Error_Handler(); }

    osThreadDef(canTxTask, BmsCanTxTask, osPriorityNormal, 0U, 256U);
    handle = osThreadCreate(osThread(canTxTask), NULL);
    if (handle == NULL) { Error_Handler(); }

    osThreadDef(faultTask, FaultTask, osPriorityRealtime, 0U, 128U);
    handle = osThreadCreate(osThread(faultTask), NULL);
    if (handle == NULL) { Error_Handler(); }
    g_fault_task_handle = (TaskHandle_t)handle;
}

static void StartDefaultTask(void const *argument)
{
    (void)argument;
    for (;;) {
        osDelay(1000U);
    }
}

static void BmsSamplingTask(void const *argument)
{
    TickType_t wake_time = xTaskGetTickCount();
    BmsSample sample;
    (void)argument;
    BmsSensorSim_Init();

    for (;;) {
        const uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        BmsSensorSim_Read(now_ms, &sample);
        if (xQueueSend(g_bms_sample_queue, &sample, 0U) != pdPASS) {
            if (g_dropped_frames < UINT8_MAX) { ++g_dropped_frames; }
        }
        vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(BMS_SAMPLE_PERIOD_MS));
    }
}

static void BmsProcessTask(void const *argument)
{
    BmsFilter filter;
    BmsProtection protection;
    BmsSample raw;
    BmsSample filtered;
    BmsSnapshot snapshot;
    BmsSnapshot loopback_snapshot;
    BmsCanFrame rx_frame;
    uint16_t previous_faults = 0U;
    (void)argument;

    memset(&loopback_snapshot, 0, sizeof(loopback_snapshot));
    BmsFilter_Init(&filter);
    BmsProtection_Init(&protection);
    if (!BmsCanTransport_Init()) {
        Error_Handler();
    }

    for (;;) {
        if (xQueueReceive(g_bms_sample_queue, &raw, pdMS_TO_TICKS(20U)) == pdPASS) {
            uint16_t faults;
            BmsFilter_Apply(&filter, &raw, &filtered);
            faults = BmsProtection_Evaluate(&protection, &filtered);
            if (g_last_can_error != 0U) {
                faults |= BMS_FAULT_CAN;
            }
            BmsSnapshot_Build(&filtered, faults, &snapshot);
            (void)xQueueOverwrite(g_bms_snapshot_queue, &snapshot);

            if ((faults & (uint16_t)~previous_faults) != 0U) {
                (void)xTaskNotify(g_fault_task_handle, faults, eSetBits);
            }
            previous_faults = faults;
        }

        while (xQueueReceive(g_can_rx_queue, &rx_frame, 0U) == pdPASS) {
            if (BmsCan_DecodeFrame(&rx_frame, &loopback_snapshot)) {
                ++g_loopback_rx_count;
            }
        }
    }
}

static void BmsCanTxTask(void const *argument)
{
    BmsSnapshot snapshot;
    BmsCanFrame frames[BMS_CAN_FRAME_COUNT];
    uint32_t i;
    (void)argument;

    for (;;) {
        if (xQueueReceive(g_bms_snapshot_queue, &snapshot, portMAX_DELAY) != pdPASS) {
            continue;
        }
        (void)BmsCan_EncodeSnapshot(&snapshot, g_dropped_frames, frames);
        for (i = 0U; i < BMS_CAN_FRAME_COUNT; ++i) {
            uint32_t attempt;
            bool sent = false;
            for (attempt = 0U; attempt < 3U; ++attempt) {
                if (BmsCanTransport_Send(&frames[i])) {
                    sent = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(1U << attempt));
            }
            if (!sent && (g_dropped_frames < UINT8_MAX)) {
                ++g_dropped_frames;
            }
        }
    }
}

static void FaultTask(void const *argument)
{
    uint32_t notification;
    (void)argument;

    for (;;) {
        if (xTaskNotifyWait(0U, UINT32_MAX, &notification, portMAX_DELAY) == pdTRUE) {
            g_last_fault_flags = (uint16_t)notification;
        }
    }
}

void BmsTasks_NotifyCanFaultFromISR(uint32_t error_code)
{
    BaseType_t higher_priority_woken = pdFALSE;
    g_last_can_error = error_code;
    if (g_fault_task_handle != NULL) {
        (void)xTaskNotifyFromISR(g_fault_task_handle, BMS_FAULT_CAN, eSetBits,
                                &higher_priority_woken);
        portYIELD_FROM_ISR(higher_priority_woken);
    }
}
