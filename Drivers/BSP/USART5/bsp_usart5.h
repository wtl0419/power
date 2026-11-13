#ifndef __BSP_USART3_H
#define __BSP_USART3_H

#include "./SYSTEM/sys/sys.h" // 包含正点原子/HAL库的基础头文件

/* 全局句柄声明，方便其他地方可能需要的调用 */
extern UART_HandleTypeDef g_uart3_handle;

/* 函数声明 */
void UART5_Init(uint32_t baudrate);
void UART5_SendString(char *str);

#endif