#ifndef __APP_CURRENT_H
#define __APP_CURRENT_H

#include "app_global.h"

/* 满量程电流 (A)，按项目实际修改 */
#define CURRENT_FULL_SCALE      100.0f

/* 把设定电流 (A) 转成 16 位数字量 */
uint16_t Current_CalcTarget(float current_a);

/* 每 1.25ms 调用一次，计算新的 g_current_ref */
uint16_t Current_Update(void);

/* 把 g_current_ref 输出到 SP9316 */
void Current_Output(uint16_t ref);
#endif