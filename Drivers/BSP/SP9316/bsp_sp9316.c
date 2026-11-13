#include "bsp_sp9316.h"
#include "./SYSTEM/delay/delay.h"

/**
 * @brief       SP9316 初始化
 *              - LATCH1: PF15，输出推挽，初始低电平
 *              - OC:     使能输出（如果硬件接了 STM32，低电平使能）
 * @note        DA0~DA15 由 74HC595 提供，不需要 STM32 直接控制
 */
void SP9316_Init(void)
{
    GPIO_InitTypeDef gpio_init;

    SP9316_LATCH_CLK_ENABLE();

    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;

    /* LATCH1 */
    gpio_init.Pin = SP9316_LATCH_PIN;
    HAL_GPIO_Init(SP9316_LATCH_PORT, &gpio_init);
    SP9316_LATCH(0);

    /* OC: 如果硬件接 STM32，初始化并使能输出 */
    /* 如果你的 OC 直接接地，把下面这段注释掉 */
#if 0
    SP9316_OC_CLK_ENABLE();
    gpio_init.Pin = SP9316_OC_PIN;
    HAL_GPIO_Init(SP9316_OC_PORT, &gpio_init);
    SP9316_Enable();
#endif
}

/**
 * @brief       给 SP9316 一个锁存脉冲
 *              上升沿触发，把 DB0~DB15 上的数据读入内部 DAC 寄存器
 * @note        调用前必须保证 74HC595 数据已稳定至少 20μs
 */
void SP9316_Latch(void)
{
    SP9316_LATCH(0);
    __NOP();
    SP9316_LATCH(1);    /* 上升沿锁存 */
    __NOP();
    SP9316_LATCH(0);
}

/**
 * @brief       使能 SP9316 输出（OC 低电平有效）
 */
void SP9316_Enable(void)
{
    SP9316_OC(0);
}

/**
 * @brief       关闭 SP9316 输出（高阻态）
 */
void SP9316_Disable(void)
{
    SP9316_OC(1);
}