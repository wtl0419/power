/**
 ****************************************************************************************************
 * @file        btim.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.2
 * @date        2024-xx-xx
 * @brief       基本定时器 驱动代码
 *              TIM6: 1.25ms 触发电流给定任务
 ****************************************************************************************************
 */

#include "./BSP/TIMER/btim.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "app_global.h"     /* 为了拿 xSem_CurrentTick */

TIM_HandleTypeDef g_tim6_handler;       /* TIM6 句柄，1.25ms */

/******************************************************************************************/
/* 中断服务函数 */

void BTIM_TIM6_INT_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_tim6_handler);
}

/**
 * @brief       定时器更新中断回调函数
 *              TIM6: 释放 xSem_CurrentTick 信号量，唤醒 current_task
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (htim == (&g_tim6_handler))         /* TIM6: 1.25ms 电流给定 */
    {
        if (xSem_CurrentTick != NULL)
        {
            xSemaphoreGiveFromISR(xSem_CurrentTick, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/******************************************************************************************/
/* 初始化函数 */

/**
 * @brief       TIM6 初始化 (1.25ms 电流给定)
 * @param       arr: 自动重装值
 * @param       psc: 预分频数
 * @note        TIM6 挂在 APB1，定时器时钟 84MHz
 *              1.25ms 中断: arr=1250-1=1249, psc=84-1=83
 *              Tout = (1249+1)*(83+1)/84M = 1250*84/84M = 1250us = 1.25ms
 */
void btim_tim6_int_init(uint16_t arr, uint16_t psc)
{
    g_tim6_handler.Instance = BTIM_TIM6_INT;
    g_tim6_handler.Init.Prescaler = psc;
    g_tim6_handler.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_tim6_handler.Init.Period = arr;
    g_tim6_handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&g_tim6_handler);

    HAL_TIM_Base_Start_IT(&g_tim6_handler);
}

/**
 * @brief       定时器底层驱动，开启时钟，设置中断优先级
 *              此函数会被 HAL_TIM_Base_Init() 调用
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == BTIM_TIM6_INT)
    {
        BTIM_TIM6_INT_CLK_ENABLE();
        HAL_NVIC_SetPriority(BTIM_TIM6_INT_IRQn, 6, 0);   /* 抢占6，FreeRTOS 安全 */
        HAL_NVIC_EnableIRQ(BTIM_TIM6_INT_IRQn);
    }
}