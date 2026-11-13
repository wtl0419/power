#include "bsp_hc165.h"
#include "./SYSTEM/delay/delay.h"
/* 静态缓存：保存最近一次 HC165_Read8() 读取到的 8 位状态 (默认上拉全1) */
static volatile uint8_t s_hc165_latest_val = 0xFF;
/**
 * @brief  初始化 74HC165 引脚
 */
void HC165_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* 1. 使能时钟 */
    HC165_CLK_ENABLE();

    /* 2. 配置 SH (PG3) 与 CLK (PG4) 为推挽输出 */
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Pin   = HC165_SH_PIN | HC165_CLK_PIN;
    HAL_GPIO_Init(GPIOG, &gpio_init);

    /* 3. 配置 DA/DS (PG5) 为输入模式 (板载有上下拉跳线，这里建议使用上拉或无上下拉) */
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Pin   = HC165_DA_PIN;
    HAL_GPIO_Init(GPIOG, &gpio_init);

    /* 4. 初始状态：空闲时 SH 为低电平，持续锁存外部引脚；CLK 为高或低皆可 */
    HC165_SH(0);
    HC165_CLK(1);
}

/**
 * @brief  按照官方时序读取 1 片 74HC165 (8位数据)
 * @retval 8位按键/输入状态
 */
uint8_t HC165_Read8(void)
{
    uint8_t i = 0;
    uint8_t da0 = 0;

    /* 1. 对应原厂代码：HC165_SH = 1 (拉高 SH，停止并行锁存，进入移位读取模式) */
    HC165_SH(1);
    delay_us(1);

    /* 2. 读取第 1 个 74HC165 的 8 位数据 */
    for (i = 0; i < 8; i++)
    {
        da0 <<= 1;

        // 低电平时采样数据 (与例程一致)
        HC165_CLK(0);
        delay_us(1);

        if (HC165_DA_READ() == GPIO_PIN_SET)
        {
            da0 |= 0x01;
        }

        // 时钟拉高产生上升沿，芯片移出下一位
        HC165_CLK(1);
        delay_us(1);
    }

    /* 3. 对应原厂代码：HC165_SH = 0 (拉低 SH，结束读取并重新锁存并行输入) */
    HC165_SH(0);
    /* ★ 更新最新读取值，供继电器状态查询函数随时安全读取 */
    s_hc165_latest_val = da0;
    return da0;
}
/**
 * @brief  判断【磁体启动交流接触器 (Bit4 / D4)】是否吸合
 * @retval 1: 吸合, 0: 断开
 */
uint8_t HC165_IsMagnetContactorClosed(void)
{
#if CONTACTOR_DEBUG_MODE
    /* 调试模式：悬空(1)为断开，接地(0)为吸合 */
    return ((s_hc165_latest_val & (1 << 4)) == 0) ? 1 : 0;
#else
    /* 实机模式：高电平(1)为吸合，低电平(0)为断开 */
    return ((s_hc165_latest_val & (1 << 4)) != 0) ? 1 : 0;
#endif
}

/**
 * @brief  判断【延时启动交流接触器 (Bit5 / D5)】是否吸合
 * @retval 1: 吸合, 0: 断开
 */
uint8_t HC165_IsDelayContactorClosed(void)
{
#if CONTACTOR_DEBUG_MODE
    /* 调试模式：悬空(1)为断开，接地(0)为吸合 */
    return ((s_hc165_latest_val & (1 << 5)) == 0) ? 1 : 0;
#else
    /* 实机模式：高电平(1)为吸合，低电平(0)为断开 */
    return ((s_hc165_latest_val & (1 << 5)) != 0) ? 1 : 0;
#endif
}