#ifndef __BTIM_H
#define __BTIM_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 基本定时器 定义 */

/* TIM3 定义 (给 lwip 用，保持原样) */
#define BTIM_TIM3_INT                   TIM3
#define BTIM_TIM3_INT_IRQn              TIM3_IRQn
#define BTIM_TIM3_INT_IRQHandler        TIM3_IRQHandler
#define BTIM_TIM3_INT_CLK_ENABLE()      do{ __HAL_RCC_TIM3_CLK_ENABLE(); }while(0)

/* TIM6 定义 (1.25ms 电流给定) */
#define BTIM_TIM6_INT                   TIM6
#define BTIM_TIM6_INT_IRQn              TIM6_DAC_IRQn
#define BTIM_TIM6_INT_IRQHandler        TIM6_DAC_IRQHandler
#define BTIM_TIM6_INT_CLK_ENABLE()      do{ __HAL_RCC_TIM6_CLK_ENABLE(); }while(0)

/******************************************************************************************/

void btim_tim3_int_init(uint16_t arr, uint16_t psc);    /* TIM3 初始化 */
void btim_tim6_int_init(uint16_t arr, uint16_t psc);    /* TIM6 初始化 */

#endif