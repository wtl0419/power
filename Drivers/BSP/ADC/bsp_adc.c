#include "bsp_adc.h"

ADC_HandleTypeDef g_adc1_handle;
DMA_HandleTypeDef g_adc1_dma_handle;

/* ADC 原始值，DMA 自动搬运到这里 */
volatile uint16_t g_adc_raw[4] = {0};

/**
 * @brief       ADC1 初始化
 *              - 4 通道扫描：PA4、PA6、PC0、PC2
 *              - DMA2 Stream0 循环搬运
 *              - 连续转换模式，启动一次后自动一直采集
 * @note        ADC 时钟 = PCLK2 / 4 = 84MHz / 4 = 21MHz
 *              12 位分辨率，采样时间 84 周期
 */
void ADC_Init(void)
{
    ADC_ChannelConfTypeDef channel_config;

    /* 1. 使能时钟 */
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 2. 配置 GPIO 为模拟输入 */
    GPIO_InitTypeDef gpio_init;
    gpio_init.Mode  = GPIO_MODE_ANALOG;
    gpio_init.Pull  = GPIO_NOPULL;

    gpio_init.Pin = ADC_CURRENT_PIN;
    HAL_GPIO_Init(ADC_CURRENT_PORT, &gpio_init);

    gpio_init.Pin = ADC_VOLTAGE_PIN;
    HAL_GPIO_Init(ADC_VOLTAGE_PORT, &gpio_init);

    gpio_init.Pin = ADC_VCE_PIN;
    HAL_GPIO_Init(ADC_VCE_PORT, &gpio_init);

    gpio_init.Pin = ADC_SPARE_PIN;
    HAL_GPIO_Init(ADC_SPARE_PORT, &gpio_init);

    /* 3. 配置 DMA2 Stream0 Channel0 (ADC1) */
    g_adc1_dma_handle.Instance                 = DMA2_Stream0;
    g_adc1_dma_handle.Init.Channel             = DMA_CHANNEL_0;
    g_adc1_dma_handle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    g_adc1_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_adc1_dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
    g_adc1_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_adc1_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    g_adc1_dma_handle.Init.Mode                = DMA_CIRCULAR;
    g_adc1_dma_handle.Init.Priority            = DMA_PRIORITY_HIGH;
    g_adc1_dma_handle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&g_adc1_dma_handle);

    __HAL_LINKDMA(&g_adc1_handle, DMA_Handle, g_adc1_dma_handle);

    /* 4. 配置 ADC1 */
    g_adc1_handle.Instance                   = ADC1;
    g_adc1_handle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;   /* 84M/4 = 21MHz */
    g_adc1_handle.Init.Resolution            = ADC_RESOLUTION_12B;
    g_adc1_handle.Init.ScanConvMode          = ENABLE;                     /* 多通道扫描 */
    g_adc1_handle.Init.ContinuousConvMode    = ENABLE;                     /* 连续转换 */
    g_adc1_handle.Init.DiscontinuousConvMode = DISABLE;
    g_adc1_handle.Init.NbrOfDiscConversion   = 0;
    g_adc1_handle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    g_adc1_handle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    g_adc1_handle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    g_adc1_handle.Init.NbrOfConversion       = 4;
    g_adc1_handle.Init.DMAContinuousRequests = ENABLE;
    g_adc1_handle.Init.EOCSelection          = ADC_EOC_SEQ_CONV;
    HAL_ADC_Init(&g_adc1_handle);

    /* 5. 配置 4 个通道 */
    channel_config.SamplingTime = ADC_SAMPLETIME_84CYCLES;

    channel_config.Channel = ADC_CURRENT_CHANNEL;
    channel_config.Rank    = 1;
    HAL_ADC_ConfigChannel(&g_adc1_handle, &channel_config);

    channel_config.Channel = ADC_VOLTAGE_CHANNEL;
    channel_config.Rank    = 2;
    HAL_ADC_ConfigChannel(&g_adc1_handle, &channel_config);

    channel_config.Channel = ADC_VCE_CHANNEL;
    channel_config.Rank    = 3;
    HAL_ADC_ConfigChannel(&g_adc1_handle, &channel_config);

    channel_config.Channel = ADC_SPARE_CHANNEL;
    channel_config.Rank    = 4;
    HAL_ADC_ConfigChannel(&g_adc1_handle, &channel_config);

    /* 6. ADC 校准 */
    //HAL_ADCEx_Calibration_Start(&g_adc1_handle);

    /* 7. 启动 ADC + DMA，之后会自动循环采集 */
    HAL_ADC_Start_DMA(&g_adc1_handle, (uint32_t *)g_adc_raw, 4);
}

/**
 * @brief       把 ADC 原始值转换为电压 (V)
 * @param       raw: ADC 原始值 0~4095
 * @retval      电压值 0~3.3V
 */
float ADC_ToVoltage(uint16_t raw)
{
    return (float)raw * 3.3f / 4095.0f;
}

/**
 * @brief       ADC MspInit，配置底层时钟和引脚
 *              会被 HAL_ADC_Init() 调用
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        __HAL_RCC_ADC1_CLK_ENABLE();
        /* 时钟和 GPIO 已在 ADC_Init 中配置，此处仅做辅助 */
    }
}

/**
 * @brief       ADC MspDeInit
 */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        __HAL_RCC_ADC1_CLK_DISABLE();
    }
}

/**
 * @brief       DMA 传输错误回调（可选，调试用）
 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    /* 调试时可以置个标志或点亮 LED */
    (void)hadc;
}