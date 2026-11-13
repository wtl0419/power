#ifndef __BSP_HC165_H
#define __BSP_HC165_H

#include "./SYSTEM/sys/sys.h"

/* -------------------------------------------------------------
 * 74HC165 引脚定义（对应模块左侧端口）
 * ------------------------------------------------------------- */
#define HC165_CLK_ENABLE()      do { __HAL_RCC_GPIOG_CLK_ENABLE(); } while(0)

// SH (锁存控制端) -> 接模块左侧 SH (PG3)
#define HC165_SH_PORT           GPIOG
#define HC165_SH_PIN            GPIO_PIN_3

// CLK (时钟输入端) -> 接模块左侧 CLK (PG4)
#define HC165_CLK_PORT          GPIOG
#define HC165_CLK_PIN           GPIO_PIN_4

// DA/DS (串行数据输出端) -> 接模块左侧 DS (PG5)
#define HC165_DA_PORT           GPIOG
#define HC165_DA_PIN            GPIO_PIN_5

/* 控制宏 */
#define HC165_SH(x)   do { (x) ? HAL_GPIO_WritePin(HC165_SH_PORT, HC165_SH_PIN, GPIO_PIN_SET) : \
                                 HAL_GPIO_WritePin(HC165_SH_PORT, HC165_SH_PIN, GPIO_PIN_RESET); } while(0)

#define HC165_CLK(x)  do { (x) ? HAL_GPIO_WritePin(HC165_CLK_PORT, HC165_CLK_PIN, GPIO_PIN_SET) : \
                                 HAL_GPIO_WritePin(HC165_CLK_PORT, HC165_CLK_PIN, GPIO_PIN_RESET); } while(0)

#define HC165_DA_READ()  HAL_GPIO_ReadPin(HC165_DA_PORT, HC165_DA_PIN)
/* ★ 接触器反馈极性模式开关：
 * 1: 调试模式 (74HC165悬空为1代表断开，接地为0代表吸合)
 * 0: 实机模式 (1代表吸合，0代表断开)
 */
#define CONTACTOR_DEBUG_MODE    1
/* 函数声明 */
void HC165_Init(void);
uint8_t HC165_Read8(void);

uint8_t HC165_IsMagnetContactorClosed(void); /* Bit4: 磁体启动交流接触器 */
uint8_t HC165_IsDelayContactorClosed(void);  /* Bit5: 延时启动交流接触器 */
#endif