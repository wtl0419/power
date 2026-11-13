/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-5-04
 * @brief       lwIP SOCKET TCPClient 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./USMART/usmart.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SRAM/sram.h"
#include "./MALLOC/malloc.h"
#include "touch.h"
#include "freertos_demo.h"
#include "bsp_usart5.h"
#include "bsp_hc595.h"
#include "app_current.h"
#include "bsp_hc165.h"
int main(void)
{
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 设置时钟,168Mhz */
    delay_init(168);                    /* 延时初始化 */
    usart_init(115200);                 /* 串口初始化为115200 */
		UART5_Init(115200);								/* 初始化串口3，波特率 115200 */
    usmart_dev.init(84);                /* 初始化USMART */
    led_init();                         /* 初始化LED */
		LCD_Init();     
		Touch_Init();   
//74hc595 test
//		HC595_Init();
//	//HC595_Write16(0x1FFF);
//	//Current_Output(0x8000); 
//	uint16_t current_value = 0;
//	while(1)
//    {
//				delay_ms(1000);
//        Current_Output(current_value);  // 全 1
//        delay_ms(1000);         // 停留 1 秒（方便万用表测）
//       current_value += 1000;
//        
//    }
//74hc165 test
//		HC165_Init();
//		uint8_t hc165_val = 0;
//		while (1)
//    {
//        // 读取 8 路状态
//        hc165_val = HC165_Read8();

//        // 翻转 LED0 指示程序正在持续运行
//        LED0_TOGGLE();

//        // 串口打印十六进制以及 8 位二进制状态 (Bit7 ~ Bit0)
//        printf("HC165 Value: 0x%02X  [BIN: ", hc165_val);
//        for (int8_t i = 7; i >= 0; i--)
//        {
//            printf("%d", (hc165_val >> i) & 0x01);
//            if (i == 4) printf(" "); // 空格分隔，方便阅读
//        }
//        printf("]\r\n");

//        // 延时 200ms 刷新一次，方便观察
//        delay_ms(200);
//    }
    freertos_demo();                    /* 创建lwIP的任务函数 */
}
