#include "bsp_usart5.h" // 记得把对应的头文件也改名
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

UART_HandleTypeDef g_uart5_handle;
uint8_t g_u5_rx_byte; 

/**
 * @brief       UART5 初始化函数
 * @param       baudrate: 波特率 (例如 9600)
 */
void UART5_Init(uint32_t baudrate)
{
    g_uart5_handle.Instance          = UART5;
    g_uart5_handle.Init.BaudRate     = baudrate;
    g_uart5_handle.Init.WordLength   = UART_WORDLENGTH_8B;   
    g_uart5_handle.Init.StopBits     = UART_STOPBITS_1;      
    g_uart5_handle.Init.Parity       = UART_PARITY_NONE;     
    g_uart5_handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;  
    g_uart5_handle.Init.Mode         = UART_MODE_TX_RX;      
    
    if (HAL_UART_Init(&g_uart5_handle) != HAL_OK)
    {
        while(1); 
    }
    
    /* 开启接收中断 */
    HAL_UART_Receive_IT(&g_uart5_handle, &g_u5_rx_byte, 1);
}

/**
 * @brief       UART5 中断服务函数
 */
void UART5_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart5_handle);
}