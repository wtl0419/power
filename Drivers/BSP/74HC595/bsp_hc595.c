#include "bsp_hc595.h"
#include "./SYSTEM/delay/delay.h"

/* 内部静态缓存：保存当前第1、2片的 DAC 值与第3片的继电器状态 */
static uint16_t s_dac_val   = 0xFFFF; /* 初始 DAC 值 (对应 ref=0 取反后的 0xFFFF) */
static uint8_t  s_relay_val = 0x00;   /* 初始继电器状态 (0x00 全断开) */

/**
 * @brief  内部函数：移位发送 8 位数据 (MSB 高位先发)
 */
static void HC595_ShiftByte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (byte & 0x80) {
            HC595_DS(1);
        } else {
            HC595_DS(0);
        }
        
        byte <<= 1;

        /* 产生移位时钟上升沿 (LDSCK) */
        HC595_SHCP(0);
        __NOP();
        HC595_SHCP(1);
        __NOP();
        HC595_SHCP(0);
    }
}

/**
 * @brief  给 595 一个锁存脉冲 (LDSTR)，将 24 位移位寄存器数据同步更新到输出引脚
 */
void HC595_Latch(void)
{
    HC595_STCP(0);
    delay_us(1);       /* 保证数据在锁存前有建立时间 */
    
    HC595_STCP(1);     /* 上升沿锁存 */
    delay_us(20);      /* 严格满足 20μs 脉宽 */
    
    HC595_STCP(0);
}

/**
 * @brief  内部核心刷新函数：
 *         1. 先发 8位 继电器数据 -> 推入最远的第 3 片
 *         2. 再发 16位 DAC 数据  -> 留在第 2 片和第 1 片
 *         3. 统一锁存输出
 */
static void HC595_UpdateAll(void)
{
    /* 1. 先发第 3 片：8位继电器控制字 */
    HC595_ShiftByte(s_relay_val);

    /* 2. 再发第 1、2 片：16位 DAC 数值 (先发高8位，再发低8位) */
    HC595_ShiftByte((uint8_t)(s_dac_val >> 8));
    HC595_ShiftByte((uint8_t)(s_dac_val & 0xFF));

    /* 3. 24位全部就位后，产生锁存脉冲更新输出 */
    HC595_Latch();
}

/**
 * @brief  初始化 74HC595 的三个控制引脚
 */
void HC595_Init(void)
{
    GPIO_InitTypeDef gpio_init;

    /* 1. 使能 GPIOE 时钟 */
    HC595_DS_CLK_ENABLE(); 

    /* 2. 配置 PE7, PE8, PE9 为推挽输出 */
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio_init.Pin = HC595_DS_PIN | HC595_SHCP_PIN | HC595_STCP_PIN;
    HAL_GPIO_Init(GPIOE, &gpio_init);

    /* 3. 初始状态全拉低 */
    HC595_DS(0);
    HC595_SHCP(0);
    HC595_STCP(0);

    /* 4. 上电将初始状态刷入 3 片 595 */
    HC595_UpdateAll();
}

/**
 * @brief  向第 1、2 片 74HC595 写入 16 位 DAC 数据
 * @param  dac_data: 16位 DAC 数据 (保持你原有的调用习惯不变)
 */
void HC595_Write16(uint16_t dac_data)
{
    s_dac_val = dac_data;
    HC595_UpdateAll();
}

/**
 * @brief  向第 3 片 74HC595 写入 8 位数据，开启/关闭对应位的继电器
 * @param  relay_state: 8位继电器状态 (Bit0~Bit7 对应 8 个继电器，1开启，0关闭)
 * @note   若继电器硬件也是反相驱动(低电平吸合)，将下方改为 s_relay_val = ~relay_state; 即可
 */
void HC595_WriteRelay8(uint8_t relay_state)
{
    s_relay_val = relay_state;
    HC595_UpdateAll();
}