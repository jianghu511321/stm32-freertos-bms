#ifndef STM32F1XX_IT_H
#define STM32F1XX_IT_H

#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void DebugMon_Handler(void);
void SysTick_Handler(void);
void TIM4_IRQHandler(void);
void USB_LP_CAN1_RX0_IRQHandler(void);
void CAN1_SCE_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif

