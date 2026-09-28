#include "stm32f1xx_it.h"

#include "bms_can_transport.h"
#include "cmsis_os.h"
#include "main.h"

void NMI_Handler(void) {}

void HardFault_Handler(void)
{
    for (;;) {}
}

void MemManage_Handler(void)
{
    for (;;) {}
}

void BusFault_Handler(void)
{
    for (;;) {}
}

void UsageFault_Handler(void)
{
    for (;;) {}
}

void DebugMon_Handler(void) {}

void SysTick_Handler(void)
{
    osSystickHandler();
}

void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim4);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan);
}

void CAN1_SCE_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *handle)
{
    if (handle->Instance == CAN1) {
        BmsCanTransport_OnRxPendingFromISR();
    }
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *handle)
{
    if (handle->Instance == CAN1) {
        BmsCanTransport_OnErrorFromISR(HAL_CAN_GetError(handle));
    }
}

