#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* ADC1 通道定义 */

/* 电流：PA4 -> ADC1_IN4 */
#define ADC_CURRENT_CHANNEL     ADC_CHANNEL_4
#define ADC_CURRENT_PORT        GPIOA
#define ADC_CURRENT_PIN         GPIO_PIN_4

/* 电压：PA6 -> ADC1_IN6 */
#define ADC_VOLTAGE_CHANNEL     ADC_CHANNEL_6
#define ADC_VOLTAGE_PORT        GPIOA
#define ADC_VOLTAGE_PIN         GPIO_PIN_6

/* 管压降：PC0 -> ADC1_IN10 */
#define ADC_VCE_CHANNEL         ADC_CHANNEL_10
#define ADC_VCE_PORT            GPIOC
#define ADC_VCE_PIN             GPIO_PIN_0

/* 预留：PC2 -> ADC1_IN12 */
#define ADC_SPARE_CHANNEL       ADC_CHANNEL_12
#define ADC_SPARE_PORT          GPIOC
#define ADC_SPARE_PIN           GPIO_PIN_2

/******************************************************************************************/
/* 传感器变比，按实际硬件修改 */

/* 电流传感器：假设 1V 对应 10A */
#define ADC_CURRENT_SCALE       10.0f
/* 电压传感器：假设 1V 对应 20V */
#define ADC_VOLTAGE_SCALE       20.0f
/* 管压降：假设 1V 对应 5V */
#define ADC_VCE_SCALE           5.0f

/******************************************************************************************/
/* 对外接口 */

/* ADC 原始值数组，顺序：[电流, 电压, 管压降, 预留] */
extern volatile uint16_t g_adc_raw[4];

void  ADC_Init(void);
float ADC_ToVoltage(uint16_t raw);

#endif