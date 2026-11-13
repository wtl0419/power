#include "app_global.h"

volatile SystemState g_sys_state = SYS_INIT;
volatile uint16_t    g_current_ref = 0;
volatile uint16_t    g_current_target = 0;
volatile float       g_current = 0;
volatile float       g_voltage = 0;
volatile float       g_vce = 0;

volatile uint16_t    g_di = 0;
volatile uint32_t    g_fault_flags = 0;
volatile uint32_t g_current_cnt = 0;
QueueHandle_t g_cmd_queue = NULL;
SemaphoreHandle_t xSem_CurrentTick = NULL;
volatile uint32_t g_power_logic_out = 0x00; // 上电默认全为0（例如：电源关，报警关）
/* 在全局变量区域添加 */
char g_net_rx_str[64] = "Waiting..."; /* 用于LCD显示的网络接收字符串 */
/* 全局硬件状态寄存器，初始化为没有任何状态 */
volatile uint32_t g_sys_status = STATUS_NONE;