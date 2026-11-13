#ifndef __BSP_SP9316_H
#define __BSP_SP9316_H

#include "./SYSTEM/sys/sys.h"

/* SP9316 控制引脚定义（从 P7 排针引出） */

/* LATCH1: 锁存信号，上升沿把 DB0~DB15 数据读入 DAC 寄存器 */
#define SP9316_LATCH_PORT           GPIOF
#define SP9316_LATCH_PIN            GPIO_PIN_15
#define SP9316_LATCH_CLK_ENABLE()   do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)

/* OC: 输出控制，低电平使能 DAC 输出
 *     如果你的硬件把 OC 直接接地，就不用软件控制，可以忽略这个引脚
 *     如果 OC 接到了 STM32，需要在初始化后拉低
 */
#define SP9316_OC_PORT              GPIOF
#define SP9316_OC_PIN               GPIO_PIN_11    /* 注意：PF11 被触摸占用，此处仅作示例 */
#define SP9316_OC_CLK_ENABLE()      do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)

/* 引脚操作宏 */
#define SP9316_LATCH(x)     do{ x ? \
                                HAL_GPIO_WritePin(SP9316_LATCH_PORT, SP9316_LATCH_PIN, GPIO_PIN_SET) : \
                                HAL_GPIO_WritePin(SP9316_LATCH_PORT, SP9316_LATCH_PIN, GPIO_PIN_RESET); \
                            }while(0)

#define SP9316_OC(x)        do{ x ? \
                                HAL_GPIO_WritePin(SP9316_OC_PORT, SP9316_OC_PIN, GPIO_PIN_SET) : \
                                HAL_GPIO_WritePin(SP9316_OC_PORT, SP9316_OC_PIN, GPIO_PIN_RESET); \
                            }while(0)

/* 函数声明 */
void SP9316_Init(void);
void SP9316_Latch(void);
void SP9316_Enable(void);
void SP9316_Disable(void);

#endif