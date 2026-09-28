#include "main.h"

TIM_HandleTypeDef htim4;

HAL_StatusTypeDef HAL_InitTick(uint32_t tick_priority)
{
#if BMS_CPU_SIMULATOR
    (void)tick_priority;
    return HAL_OK;
#else
    RCC_ClkInitTypeDef clock_config;
    uint32_t flash_latency;
    uint32_t pclk1;
    uint32_t timer_clock;

    __HAL_RCC_TIM4_CLK_ENABLE();
    HAL_RCC_GetClockConfig(&clock_config, &flash_latency);
    pclk1 = HAL_RCC_GetPCLK1Freq();
    timer_clock = (clock_config.APB1CLKDivider == RCC_HCLK_DIV1) ? pclk1 : (2U * pclk1);

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = (timer_clock / 1000000U) - 1U;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 999U;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_NVIC_SetPriority(TIM4_IRQn, tick_priority, 0U);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    return HAL_TIM_Base_Start_IT(&htim4);
#endif
}

void HAL_SuspendTick(void)
{
#if !BMS_CPU_SIMULATOR
    __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_UPDATE);
#endif
}

void HAL_ResumeTick(void)
{
#if !BMS_CPU_SIMULATOR
    __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
#endif
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *handle)
{
    if (handle->Instance == TIM4) {
        HAL_IncTick();
    }
}

