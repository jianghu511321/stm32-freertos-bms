#include "bms_can_transport.h"

#include <string.h>

#include "bms_config.h"
#include "bms_freertos.h"
#include "main.h"

bool BmsCanTransport_Init(void)
{
#if BMS_TRANSPORT_SOFTWARE_LOOPBACK
    return true;
#else
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = 0U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0U;
    filter.FilterIdLow = 0U;
    filter.FilterMaskIdHigh = 0U;
    filter.FilterMaskIdLow = 0U;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;

    if (HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK) {
        return false;
    }
    if (HAL_CAN_Start(&hcan) != HAL_OK) {
        return false;
    }
    return HAL_CAN_ActivateNotification(
        &hcan,
        CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF |
        CAN_IT_LAST_ERROR_CODE) == HAL_OK;
#endif
}

bool BmsCanTransport_Send(const BmsCanFrame *frame)
{
    if ((frame == NULL) || (frame->dlc > 8U)) {
        return false;
    }

#if BMS_TRANSPORT_SOFTWARE_LOOPBACK
    return xQueueSend(g_can_rx_queue, frame, 0U) == pdPASS;
#else
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox;
    header.StdId = frame->id;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = frame->dlc;
    header.TransmitGlobalTime = DISABLE;
    return HAL_CAN_AddTxMessage(&hcan, &header, (uint8_t *)frame->data, &mailbox) == HAL_OK;
#endif
}

void BmsCanTransport_OnRxPendingFromISR(void)
{
#if !BMS_TRANSPORT_SOFTWARE_LOOPBACK
    CAN_RxHeaderTypeDef header;
    BmsCanFrame frame;
    BaseType_t higher_priority_woken = pdFALSE;

    if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &header, frame.data) != HAL_OK) {
        return;
    }
    if ((header.IDE != CAN_ID_STD) || (header.DLC > 8U)) {
        return;
    }
    frame.id = (uint16_t)header.StdId;
    frame.dlc = (uint8_t)header.DLC;
    (void)xQueueSendFromISR(g_can_rx_queue, &frame, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
#endif
}

void BmsCanTransport_OnErrorFromISR(uint32_t error_code)
{
    BmsTasks_NotifyCanFaultFromISR(error_code);
}
