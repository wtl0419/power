#ifndef __APP_GLOBAL_H
#define __APP_GLOBAL_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#define FAULT_NONE              0x00000000
#define FAULT_TEMP_BRIDGE       (1 << 0)    /* Bit0: 温度故障 (DI: 0故障, 1正常) */
#define FAULT_WATER             (1 << 1)    /* Bit1: 水流故障 (DI) */
#define FAULT_APB               (1 << 2)    /* Bit2: APB故障  (DI) */
#define FAULT_ESTOP             (1 << 3)    /* Bit3: 急停按下 (DI) */
#define FAULT_LOAD              (1 << 4)    /* Bit4: 负载故障 (ADC: g_vce > 7V 持续 5s) */
#define FAULT_OVER_CURRENT      (1 << 5)    /* Bit5: 电流故障 (ADC: g_current > 22A 持续 5s) */
#define FAULT_OVER_VOLTAGE      (1 << 6)    /* Bit6: 电压故障 (ADC: g_voltage > 7V) */



extern volatile uint32_t g_fault_flags; // 全局故障寄存器
/* 全局输出控制寄存器 (包含逻辑输出和 DAC 数据) */
extern volatile uint32_t g_power_logic_out; 

/* ---- 逻辑控制输出宏定义 ---- */
#define LOGIC_CTRL_MAGNET_ON    (1 << 0)  // 第0位：控制磁体启动交流接触器 (1:吸合, 0:断开)
#define LOGIC_CTRL_DELAY_ON     (1 << 1)  // 第1位：控制延时启动交流接触器 (1:吸合, 0:断开)
#define LOGIC_CTRL_ALARM        (1 << 2)  // 第2位：控制系统声光报警器 (1:响亮/亮起, 0:关闭)

// 如果还有其他外部控制设备，可以继续按顺序添加
#define LOGIC_CTRL_FAN_ON       (1 << 3)  // 第3位：控制散热风扇
#define LOGIC_CTRL_PUMP_ON      (1 << 4)  // 第4位：控制水泵
/* ---- 2. 系统状态反馈掩码 (用于 g_sys_status) ---- */
#define STATUS_NONE                 0x00000000
#define STATUS_MAGNET_CONTACTOR     (1 << 6)  // 0.6: 磁体启动交流接触器反馈
#define STATUS_DELAY_CONTACTOR      (1 << 8)  // 1.0: 延时启动交流接触器反馈

extern volatile uint32_t g_sys_status;  // 全局硬件状态寄存器，存储非故障类的反馈


/* 系统状态 */
typedef enum {
    SYS_INIT = 0,
    SYS_IDLE,
    SYS_STARTING,
    SYS_RUN,
    SYS_STOPPING,
    SYS_FAULT,
    SYS_RESET
} SystemState;


/* 命令类型 */
#define CMD_START       0x01
#define CMD_STOP        0x02
#define CMD_RESET       0x03
#define CMD_SET_CUR     0x04

/* 全局变量（定义在 app_global.c 里） */
extern volatile SystemState g_sys_state;
extern volatile uint16_t    g_current_ref;      /* 当前DAC输出值 */
extern volatile uint16_t    g_current_target;   /* 目标电流数字值 */
extern volatile float       g_current;          /* 实测电流 */
extern volatile float       g_voltage;          /* 实测电压 */
extern volatile float       g_vce;              /* 实测管压降 */
extern volatile uint16_t    g_di;               /* 12路数字输入 */
extern volatile uint32_t    g_fault_flags;      /* 故障位 */
extern volatile uint32_t g_current_cnt;
extern volatile uint32_t g_sys_status;
/* 队列 */
extern QueueHandle_t g_cmd_queue;

/* 信号量 */
extern SemaphoreHandle_t xSem_CurrentTick;      /* 1.25ms 定时信号 */
extern char g_net_rx_str[64];
#endif