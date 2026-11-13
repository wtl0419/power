#include "app_current.h"
#include "bsp_hc595.h"
#include "bsp_sp9316.h"
#include "./SYSTEM/delay/delay.h"
#include "app_global.h"

#define FULL_SCALE_CURRENT_A  20.0f  // 满量程电流 (20A)

/* 将安培数转为 16位数值 */
uint16_t Current_CalcTarget(float current_A)
{
    float ratio;
    uint32_t val;

    if (current_A < 0.0f) current_A = 0.0f;
    if (current_A > FULL_SCALE_CURRENT_A) current_A = FULL_SCALE_CURRENT_A;

    ratio = current_A / FULL_SCALE_CURRENT_A;
    val = (uint32_t)(ratio * 65535.0f + 0.5f);

    return (uint16_t)val;
}
uint16_t Current_Update(void)
{
    switch (g_sys_state)
    {
    case SYS_RUN:
        /* ★ 修改：只有在当前值 < 目标值时，才递增 */
        if (g_current_ref < g_current_target)
        {
            g_current_ref++;
        }
        /* 如果当前值 > 目标值（比如中途改小了目标值），则递减 */
        else if (g_current_ref > g_current_target)
        {
            if (g_current_ref > 0) g_current_ref--;
        }
        break;

    case SYS_STOPPING:
    case SYS_FAULT:
        /* 停机或故障：不管目标值多少，全部降到 0 */
        if (g_current_ref > 0)  
        {
            g_current_ref--;    
        }
        break;

    default:
        g_current_ref = 0;
        break;
    }
    return g_current_ref;
}
/* 发送给 595 */
void Current_Output(uint16_t ref)
{
    /* 硬件反相，软件取反 */
    uint16_t hc595_data = ~ref; 
    HC595_Write16(hc595_data);
}