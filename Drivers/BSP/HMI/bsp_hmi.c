#include "bsp_hmi.h"
#include "bsp_usart5.h"
#include "app_current.h"  /* 为了调用 Current_CalcTarget() */
#include "app_global.h"   /* 为了使用 g_cmd_queue 和 CMD_START/CMD_STOP */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>       /* 必须包含，为了使用 atof() */
#include "bsp_hc165.h"
extern UART_HandleTypeDef g_uart5_handle;

/* ---------------- 环形缓冲区定义 ---------------- */
#define HMI_RX_BUF_SIZE 128
static uint8_t  g_hmi_rx_buf[HMI_RX_BUF_SIZE];
static uint16_t g_hmi_rx_head = 0; // 写指针（中断操作）
static uint16_t g_hmi_rx_tail = 0; // 读指针（任务操作）

/**
 * @brief  串口中断回调中调用，快速存入环形缓冲区
 */
void HMI_Rx_Byte(uint8_t byte)
{
    uint16_t next_head = (g_hmi_rx_head + 1) % HMI_RX_BUF_SIZE;
    if (next_head != g_hmi_rx_tail) 
    {
        g_hmi_rx_buf[g_hmi_rx_head] = byte;
        g_hmi_rx_head = next_head;
    }
}

/**
 * @brief  在 display_task 中调用，解析缓冲区数据
 */
void HMI_Process_Cmd(void)
{
    static uint8_t rx_state = 0;       // 解析状态
    static uint8_t rx_cmd_type = 0;    // 缓存的命令字
    static char str_buf[32];           // 字符串缓存
    static uint8_t str_idx = 0;        // 字符串索引

    uint8_t byte;
    
    /* 只要缓冲区里还有数据，就一直读 */
    while (g_hmi_rx_tail != g_hmi_rx_head) 
    {
        byte = g_hmi_rx_buf[g_hmi_rx_tail];
        g_hmi_rx_tail = (g_hmi_rx_tail + 1) % HMI_RX_BUF_SIZE;

        switch (rx_state)
        {
        /* ================= 1. 原有的按键控制 (55 01 FF FF FF) ================= */
        case 0: /* 等待帧头 0x55 或 字符 'S' */
            if (byte == 0x55) { 
                rx_state = 1; 
            }
            else if (byte == 'S') { 
                str_idx = 0; 
                str_buf[str_idx++] = byte; 
                rx_state = 10; // 进入字符串接收模式
            }
            break;
            
        case 1: /* 接收命令字 */
            if (byte == 0x01 || byte == 0x02 || byte == 0x03) {
                rx_cmd_type = byte;
                rx_state = 2;
            } else {
                rx_state = 0;
            }
            break;
            
        case 2: rx_state = (byte == 0xFF) ? 3 : 0; break;
        case 3: rx_state = (byte == 0xFF) ? 4 : 0; break;
        case 4: 
            if (byte == 0xFF) {
                uint8_t queue_cmd = 0;
                if (rx_cmd_type == 0x01) queue_cmd = CMD_START;
                if (rx_cmd_type == 0x02) queue_cmd = CMD_STOP;
                if (rx_cmd_type == 0x03) queue_cmd = CMD_RESET; //复位故障
                if (queue_cmd != 0) {
                    xQueueSend(g_cmd_queue, &queue_cmd, 0);
                }
            }
            rx_state = 0; 
            break;

        /* ================= 2. 电流设定 (SET:10.50 FF FF FF) ================= */
        case 10: 
            if (str_idx < 31) {
                str_buf[str_idx++] = byte;
            }
            
            // 检测是否收到了 3 个 0xFF
            if (str_idx >= 3 && (uint8_t)str_buf[str_idx-1] == 0xFF && (uint8_t)str_buf[str_idx-2] == 0xFF && (uint8_t)str_buf[str_idx-3] == 0xFF) 
            {
                // 截断结束符
                str_buf[str_idx - 3] = '\0'; 
                
                // 解析字符串：检查前 4 个字符是不是 "SET:"
                if (strncmp(str_buf, "SET:", 4) == 0) 
                {
                    // 提取 "SET:" 后面的数字，转成浮点数 (完美支持文本 "10.5")
                    float set_val = atof(&str_buf[4]);
                    
                    // 防御性编程：如果屏幕发来乱码导致转出负数或极大值
                    if (set_val >= 0.0f && set_val <= 20.0f) 
                    {
                        // 调用你的计算函数，转换为 16 位 DAC 值
                        g_current_target = Current_CalcTarget(set_val);
                        printf("HMI Set: %.2fA, DAC Target: %d\r\n", set_val, g_current_target);
                    }
                    else
                    {
                        printf("Invalid Current Value!\r\n");
                    }
                }
                rx_state = 0; // 解析完毕，回归状态 0
            }
            break;
            
        default:
            rx_state = 0;
            break;
        }
    }
}
/* 轻量级寄存器发送函数，避免 HAL_UART_Transmit 锁死接收中断 */
static void UART5_Send_Raw(const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        while ((UART5->SR & 0x40) == 0); /* 等待发送完成 */
        UART5->DR = buf[i];
    }
}

void HMI_Init(void)
{
    /* 发送结束符清空缓冲区，并设置 bkcmd=0 关闭屏幕的执行结果/错误码回传 */
    const char init_cmd[] = "\xff\xff\xff" "bkcmd=0\xff\xff\xff";
    UART5_Send_Raw((const uint8_t *)init_cmd, sizeof(init_cmd) - 1);
}

void HMI_Send_String(char *obj_name, char *show_str)
{
    char send_buf[64];
    sprintf(send_buf, "%s.txt=\"%s\"\xff\xff\xff", obj_name, show_str);
    HAL_UART_Transmit(&g_uart5_handle, (uint8_t *)send_buf, strlen(send_buf), 50);
}

void HMI_Update_VI(float voltage, float current)
{
    char str_buf[32];
    sprintf(str_buf, "%.2fV", voltage);
    HMI_Send_String("Operation_page.voltage_val", str_buf);
    sprintf(str_buf, "%.2fA", current);
    HMI_Send_String("Operation_page.current_val", str_buf);
}

/* 内部标志位：0-无提示请求, 1-请求提示"复位成功", 2-请求提示"存在未清除故障" */
static volatile uint8_t s_reset_tip_req = 0;

/**
 * @brief  供 state_task 在按下复位键判定结束后调用
 * @param  is_success: 1 表示故障已全部清除复位成功；0 表示仍存在未清除的硬件故障
 */
void HMI_Request_Reset_Tip(uint8_t is_success)
{
    if (is_success) {
        s_reset_tip_req = 1;
    } else {
        s_reset_tip_req = 2;
    }
}

/**
 * @brief  刷新故障页面的 6 项状态 (已彻底去除继电器1、2)
 */
void HMI_Update_Fault_Page(uint32_t fault_flags)
{
    char send_buf[768];
    char *ptr = send_buf;

    char *txt_temp, *txt_water, *txt_apb, *txt_stop, *txt_load, *txt_curr, *txt_volt;
    char *col_temp, *col_water, *col_apb, *col_stop, *col_load, *col_curr, *col_volt;

    /* 1. 温度检测 (DI: 0故障, 1正常) */
    if (fault_flags & FAULT_TEMP_BRIDGE) {
        txt_temp = "温度故障"; col_temp = "RED";
    } else {
        txt_temp = "温度正常"; col_temp = "WHITE";
    }

    /* 2. 水流检测 (water_status) */
    if (fault_flags & FAULT_WATER) {
        txt_water = "水流故障"; col_water = "RED";
    } else {
        txt_water = "水流正常"; col_water = "WHITE";
    }

    /* 3. APB检测 (apb_status) */
    if (fault_flags & FAULT_APB) {
        txt_apb = "APB故障"; col_apb = "RED";
    } else {
        txt_apb = "APB正常"; col_apb = "WHITE";
    }

    /* 4. 急停检测 (stop_status) */
    if (fault_flags & FAULT_ESTOP) {
        txt_stop = "急停按下"; col_stop = "RED";
    } else {
        txt_stop = "急停正常"; col_stop = "WHITE";
    }

    /* 5. 负载检查 (reload_status，由模拟量 g_vce > 7V 持续 5s 触发) */
    if (fault_flags & FAULT_LOAD) {
        txt_load = "负载故障"; col_load = "RED";
    } else {
        txt_load = "负载正常"; col_load = "WHITE";
    }

    /* 6. 电流检测 (current_status，g_current > 22A 持续 5s 触发) */
    if (fault_flags & FAULT_OVER_CURRENT) {
        txt_curr = "电流故障"; col_curr = "RED";
    } else {
        txt_curr = "电流正常"; col_curr = "WHITE";
    }

    /* 7. 电压检测 (voltage_status，g_voltage > 7V 触发) */
    if (fault_flags & FAULT_OVER_VOLTAGE) {
        txt_volt = "电压故障"; col_volt = "RED";
    } else {
        txt_volt = "电压正常"; col_volt = "WHITE";
    }

    /* ====== 统一拼接 7 路故障颜色和文本指令 ====== */
    ptr += sprintf(ptr, "fault_page.temp_status.pco=%s\xff\xff\xff", col_temp);
    ptr += sprintf(ptr, "fault_page.temp_status.txt=\"%s\"\xff\xff\xff", txt_temp);

    ptr += sprintf(ptr, "fault_page.water_status.pco=%s\xff\xff\xff", col_water);
    ptr += sprintf(ptr, "fault_page.water_status.txt=\"%s\"\xff\xff\xff", txt_water);

    ptr += sprintf(ptr, "fault_page.apb_status.pco=%s\xff\xff\xff", col_apb);
    ptr += sprintf(ptr, "fault_page.apb_status.txt=\"%s\"\xff\xff\xff", txt_apb);

    ptr += sprintf(ptr, "fault_page.stop_status.pco=%s\xff\xff\xff", col_stop);
    ptr += sprintf(ptr, "fault_page.stop_status.txt=\"%s\"\xff\xff\xff", txt_stop);

    ptr += sprintf(ptr, "fault_page.reload_status.pco=%s\xff\xff\xff", col_load);
    ptr += sprintf(ptr, "fault_page.reload_status.txt=\"%s\"\xff\xff\xff", txt_load);

    ptr += sprintf(ptr, "fault_page.current_status.pco=%s\xff\xff\xff", col_curr);
    ptr += sprintf(ptr, "fault_page.current_status.txt=\"%s\"\xff\xff\xff", txt_curr);

    ptr += sprintf(ptr, "fault_page.voltage_status.pco=%s\xff\xff\xff", col_volt);
    ptr += sprintf(ptr, "fault_page.voltage_status.txt=\"%s\"\xff\xff\xff", txt_volt);

    /* 使用寄存器直接发送 */
    UART5_Send_Raw((uint8_t *)send_buf, (uint16_t)(ptr - send_buf));
}

/**
 * @brief  内部调用：直接刷新 fault_page.fault_describe 文本与颜色
 */
static void HMI_Send_Fault_Desc(const char *str, const char *color)
{
    char buf[128];
    /* 先清一次残余指令再发颜色和文本，确保 100% 被串口屏解析 */
    int len = sprintf(buf, "\xff\xff\xff"
                           "fault_page.fault_describe.pco=%s\xff\xff\xff"
                           "fault_page.fault_describe.txt=\"%s\"\xff\xff\xff", 
                           color, str);
    UART5_Send_Raw((const uint8_t *)buf, (uint16_t)len);
}

/**
 * @brief  在 display_task 循环中调用，专门处理复位按钮按下的提示语刷新
 */
uint8_t HMI_Check_Reset_Tip(void)
{
    if (s_reset_tip_req == 1)
    {
        s_reset_tip_req = 0;
        /* 1. 先强制用当前最新的 g_fault_flags 刷一遍 6 个标签（全部变白字“正常”） */
        HMI_Update_Fault_Page(g_fault_flags);
        /* 2. 再刷底部提示框 */
        HMI_Send_Fault_Desc("故障复位成功 ", "WHITE");
        return 1;
    }
    else if (s_reset_tip_req == 2)
    {
        s_reset_tip_req = 0;
        /* ★ 1. 复位失败时，也强制用当前最新的 g_fault_flags 刷一遍 6 个标签！
         *    确保未消除的故障标签 100% 保持红字“XX故障”，已消除的变回“XX正常”！
         */
        HMI_Update_Fault_Page(g_fault_flags);
        /* 2. 再刷底部红字提示框 */
        HMI_Send_Fault_Desc("存在未清除故障 复位失败 ", "RED");
        return 2;
    }
    return 0;
}