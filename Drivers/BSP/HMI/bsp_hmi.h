#ifndef __BSP_HMI_H
#define __BSP_HMI_H

#include "./SYSTEM/sys/sys.h"

/* 函数声明 */
void HMI_Init(void);                                    /* 串口屏初始化清空缓冲区 */
void HMI_Send_String(char *obj_name, char *show_str);   /* 向指定文本控件发送字符串 */
void HMI_Update_VI(float voltage, float current);       /* 刷新主界面的电压与电流显示 */
void HMI_Update_Fault_Page(uint32_t fault_flags);			  /* 更新故障页面的 6 路故障状态 */

void HMI_Rx_Byte(uint8_t byte);  // 供串口中断调用
void HMI_Process_Cmd(void);      // 供后台任务调用
/* 触发一次 fault_describe 提示信息更新 (线程安全，在 state_task 中调用) */
void HMI_Request_Reset_Tip(uint8_t is_success);
uint8_t HMI_Check_Reset_Tip(void);
#endif