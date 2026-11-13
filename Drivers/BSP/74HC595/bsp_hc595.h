#ifndef __BSP_HC595_H
#define __BSP_HC595_H

#include "./SYSTEM/sys/sys.h"

/* =========================================================
 * 74HC595 引脚定义 (已切换至 PE7, PE8, PE9)
 * ========================================================= */
/* LDSI (串行数据输入) -> PE7 */
#define HC595_DS_PORT           GPIOE
#define HC595_DS_PIN            GPIO_PIN_7
#define HC595_DS_CLK_ENABLE()   do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)

/* LDSCK (移位时钟) -> PE8 */
#define HC595_SHCP_PORT         GPIOE
#define HC595_SHCP_PIN          GPIO_PIN_8

/* LDSTR (锁存时钟) -> PE9 */
#define HC595_STCP_PORT         GPIOE
#define HC595_STCP_PIN          GPIO_PIN_9

/* 引脚操作宏 (直接操作寄存器，速度最快) */
#define HC595_DS(x)     do{ x ? \
                            HAL_GPIO_WritePin(HC595_DS_PORT, HC595_DS_PIN, GPIO_PIN_SET) : \
                            HAL_GPIO_WritePin(HC595_DS_PORT, HC595_DS_PIN, GPIO_PIN_RESET); \
                        }while(0)

#define HC595_SHCP(x)   do{ x ? \
                            HAL_GPIO_WritePin(HC595_SHCP_PORT, HC595_SHCP_PIN, GPIO_PIN_SET) : \
                            HAL_GPIO_WritePin(HC595_SHCP_PORT, HC595_SHCP_PIN, GPIO_PIN_RESET); \
                        }while(0)

#define HC595_STCP(x)   do{ x ? \
                            HAL_GPIO_WritePin(HC595_STCP_PORT, HC595_STCP_PIN, GPIO_PIN_SET) : \
                            HAL_GPIO_WritePin(HC595_STCP_PORT, HC595_STCP_PIN, GPIO_PIN_RESET); \
                        }while(0)

/* 函数声明 */
void HC595_Init(void);
void HC595_Write16(uint16_t data);
void HC595_Latch(void);
void HC595_WriteRelay8(uint8_t relay_state);
#endif