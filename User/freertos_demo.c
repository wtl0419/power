#include "freertos_demo.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./MALLOC/malloc.h"
#include "lwip_comm.h"
#include "lwip_demo.h"
#include "lwipopts.h"
#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "app_global.h"
#include "./BSP/TIMER/btim.h"
#include "app_current.h"
#include "bsp_hc595.h"
#include "bsp_sp9316.h"
#include "bsp_adc.h"
#include "bsp_hc165.h"
#include "bsp_usart5.h"
#include "bsp_hmi.h"
/******************************************************************************************************/
/* 任务配置 */

/* START_TASK */
#define START_TASK_PRIO         5
#define START_STK_SIZE          256
TaskHandle_t StartTask_Handler;
void start_task(void *pvParameters);

/* FAULT_TASK 故障检测 */
#define FAULT_TASK_PRIO         15
#define FAULT_STK_SIZE          256
TaskHandle_t FaultTask_Handler;
void fault_task(void *pvParameters);

/* CURRENT_TASK 电流给定 */
#define CURRENT_TASK_PRIO       9
#define CURRENT_STK_SIZE        256
TaskHandle_t CurrentTask_Handler;
void current_task(void *pvParameters);

/* STATE_TASK 状态机 */
#define STATE_TASK_PRIO         8
#define STATE_STK_SIZE          512
TaskHandle_t StateTask_Handler;
void state_task(void *pvParameters);

/* ADC_TASK */
#define ADC_TASK_PRIO           7
#define ADC_STK_SIZE            256
TaskHandle_t AdcTask_Handler;
void adc_task(void *pvParameters);

/* RS485_TASK */
#define RS485_TASK_PRIO         6
#define RS485_STK_SIZE          512
TaskHandle_t Rs485Task_Handler;
void rs485_task(void *pvParameters);

/* DISPLAY_TASK */
#define DISPLAY_TASK_PRIO       5
#define DISPLAY_STK_SIZE        512
TaskHandle_t DisplayTask_Handler;
void display_task(void *pvParameters);

/* LED_TASK */
#define LED_TASK_PRIO           4
#define LED_STK_SIZE            128
TaskHandle_t LEDTask_Handler;
void led_task(void *pvParameters);

/* ETH_TASK 以太网远程通信 */
#define ETH_TASK_PRIO           3
#define ETH_STK_SIZE            1024
TaskHandle_t EthTask_Handler;
/* CAN 相关队列 */
QueueHandle_t g_can_queue;
QueueHandle_t g_can_send_queue;
void eth_task(void *pvParameters);

/******************************************************************************************************/

void freertos_demo(void)
{
    xTaskCreate((TaskFunction_t )start_task,
                (const char *   )"start_task",
                (uint16_t       )START_STK_SIZE,
                (void *         )NULL,
                (UBaseType_t    )START_TASK_PRIO,
                (TaskHandle_t * )&StartTask_Handler);

    vTaskStartScheduler();
}
void start_task(void *pvParameters)
{
    pvParameters = pvParameters;
		printf("entry start task\r\n");
		while (lwip_comm_init() != 0)
    {
        delay_ms(500);
        LED1_TOGGLE();
    }
    int phy = ethernet_read_phy(PHY_SR);
    while (!phy)  /* 检查MCU与PHY芯片是否通信成功 */
    {
        printf("MCU与PHY芯片通信失败，请检查电路或者源码！！！！\r\n");
    }
    /* ---- 3. 创建队列和信号量 ---- */
    taskENTER_CRITICAL();

    g_cmd_queue = xQueueCreate(10, sizeof(uint8_t));
    xSem_CurrentTick = xSemaphoreCreateBinary();
		
		/* 启动 TIM6，1.25ms 周期 */
		btim_tim6_int_init(1250 - 1, 84 - 1);
    /* ---- 4. 创建任务 ---- */
    xTaskCreate((TaskFunction_t )fault_task,
                (const char*    )"fault_task",
                (uint16_t       )FAULT_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )FAULT_TASK_PRIO,
                (TaskHandle_t*  )&FaultTask_Handler);

    xTaskCreate((TaskFunction_t )current_task,
                (const char*    )"current_task",
                (uint16_t       )CURRENT_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )CURRENT_TASK_PRIO,
                (TaskHandle_t*  )&CurrentTask_Handler);

    xTaskCreate((TaskFunction_t )state_task,
                (const char*    )"state_task",
                (uint16_t       )STATE_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )STATE_TASK_PRIO,
                (TaskHandle_t*  )&StateTask_Handler);

    xTaskCreate((TaskFunction_t )adc_task,
                (const char*    )"adc_task",
                (uint16_t       )ADC_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )ADC_TASK_PRIO,
                (TaskHandle_t*  )&AdcTask_Handler);

    xTaskCreate((TaskFunction_t )rs485_task,
                (const char*    )"rs485_task",
                (uint16_t       )RS485_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )RS485_TASK_PRIO,
                (TaskHandle_t*  )&Rs485Task_Handler);

    xTaskCreate((TaskFunction_t )display_task,
                (const char*    )"display_task",
                (uint16_t       )DISPLAY_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )DISPLAY_TASK_PRIO,
                (TaskHandle_t*  )&DisplayTask_Handler);

    xTaskCreate((TaskFunction_t )led_task,
                (const char*    )"led_task",
                (uint16_t       )LED_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LED_TASK_PRIO,
                (TaskHandle_t*  )&LEDTask_Handler);

    xTaskCreate((TaskFunction_t )eth_task,
                (const char*    )"eth_task",
                (uint16_t       )ETH_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )ETH_TASK_PRIO,
                (TaskHandle_t*  )&EthTask_Handler);

    
    taskEXIT_CRITICAL();
		vTaskDelete(StartTask_Handler);
}

/******************************************************************************************************/
/* 各任务框架 */

/* LED 心跳 */
void led_task(void *pvParameters)
{
    pvParameters = pvParameters;
		uint16_t current_value = 2000;
    while (1)
    {
        LED0_TOGGLE();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
/* =====================================================================
 * ★ 故障检测参数配置
 * ===================================================================== */
#define FAULT_DEBUG_MODE        1       /* 1: 调试模式(165悬空全1正常, 接地0故障), 0: 实机模式 */

#define OVER_LOAD_LIMIT_V       7.0f    /* ★ 负载(g_vce)故障阈值：7.0V */
#define OVER_LOAD_DELAY_MS      5000    /* ★ 负载故障延时：5000ms (5秒) */

#define OVER_CURR_LIMIT_A       22.0f   /* 过流阈值：22.0A (额定最大20A) */
#define OVER_CURR_DELAY_MS      5000    /* 过流延时：5000ms (5秒) */

#define OVER_VOLT_LIMIT_V       7.0f    /* 过压阈值：7.0V (额定6V) */
#define OVER_VOLT_FILTER_MS     10      /* 过压防毛刺滤波：10ms */

/* 故障检测：1ms 周期 */
void fault_task(void *pvParameters)
{
    TickType_t last = xTaskGetTickCount();
    uint8_t current_di = 0, last_di = 0;
    uint8_t debounce_cnt = 0;

    /* 模拟量持续时间计数器 (1ms 累加 1 次) */
    uint16_t over_load_cnt = 0; /* ★ 新增：负载 g_vce 超限计时器 */
    uint16_t over_curr_cnt = 0;
    uint16_t over_volt_cnt = 0;
    
    /* 初始化 165 */
    HC165_Init(); 

    while (1)
    {
        uint32_t active_faults = FAULT_NONE;

        /* =============================================================
         * 1. 74HC165 硬件数字量故障检测 (温度、水流、APB、急停，5ms 消抖)
         * ============================================================= */
        current_di = HC165_Read8();
        
        if (current_di == last_di) {
            if (debounce_cnt < 5) debounce_cnt++;
        } else {
            debounce_cnt = 0; 
            last_di = current_di;
        }

        if (debounce_cnt >= 5)
        {
            /* ★ 温度检测 (Bit0)：已改为 0 为故障，1 为正常（调试与实机极性完全一致） */
            if ((current_di & (1 << 0)) == 0) active_faults |= FAULT_TEMP_BRIDGE;

#if FAULT_DEBUG_MODE
            /* [调试模式]：74HC165 悬空默认上拉为 1 正常，接地 0 触发故障 */
            if ((current_di & (1 << 1)) == 0) active_faults |= FAULT_WATER; // Bit1: 水流检测
            if ((current_di & (1 << 2)) == 0) active_faults |= FAULT_APB;   // Bit2: APB检测
            if ((current_di & (1 << 3)) == 0) active_faults |= FAULT_ESTOP; // Bit3: 急停检测
#else
            /* [实机模式]：若实机也是 0 故障则改为 == 0，若为 1 故障则保持 != 0 */
            if ((current_di & (1 << 1)) != 0) active_faults |= FAULT_WATER; // Bit1: 水流检测
            if ((current_di & (1 << 2)) != 0) active_faults |= FAULT_APB;   // Bit2: APB检测
            if ((current_di & (1 << 3)) != 0) active_faults |= FAULT_ESTOP; // Bit3: 急停检测
#endif
        }

        /* =============================================================
         * 2. ADC 模拟量故障检测：负载(g_vce)、电流(g_current)、电压(g_voltage)
         * ============================================================= */
        /* (1) 负载检测：g_vce 超过 7V 持续满 5s (5000ms) 报故障 (上电自检阶段 10ms 即报) */
        if (g_vce > OVER_LOAD_LIMIT_V)
        {
            if (over_load_cnt < OVER_LOAD_DELAY_MS) over_load_cnt++;
            
            if (g_sys_state == SYS_INIT) {
                if (over_load_cnt >= 10) active_faults |= FAULT_LOAD;
            } else {
                if (over_load_cnt >= OVER_LOAD_DELAY_MS) active_faults |= FAULT_LOAD;
            }
        }
        else
        {
            over_load_cnt = 0;
        }

        /* (2) 电流检测：g_current 超过 22A 持续满 5s (5000ms) 报故障 (上电自检阶段 10ms 即报) */
        if (g_current > OVER_CURR_LIMIT_A)
        {
            if (over_curr_cnt < OVER_CURR_DELAY_MS) over_curr_cnt++;
            
            if (g_sys_state == SYS_INIT) {
                if (over_curr_cnt >= 10) active_faults |= FAULT_OVER_CURRENT;
            } else {
                if (over_curr_cnt >= OVER_CURR_DELAY_MS) active_faults |= FAULT_OVER_CURRENT;
            }
        }
        else
        {
            over_curr_cnt = 0;
        }

        /* (3) 电压检测：g_voltage 超过 7V (10ms 滤波防噪点) 立即报故障 */
        if (g_voltage > OVER_VOLT_LIMIT_V)
        {
            if (over_volt_cnt < OVER_VOLT_FILTER_MS) over_volt_cnt++;
            if (over_volt_cnt >= OVER_VOLT_FILTER_MS)
            {
                active_faults |= FAULT_OVER_VOLTAGE;
            }
        }
        else
        {
            over_volt_cnt = 0;
        }

        /* =============================================================
         * 3. 触发故障锁存与状态切换
         * ============================================================= */
        if (active_faults != FAULT_NONE)
        {
            g_fault_flags |= active_faults;
            
            /* 若不在上电自检阶段（自检阶段由 state_task 统一跳转），直接切入 SYS_FAULT 停机 */
            if (g_sys_state != SYS_FAULT && g_sys_state != SYS_INIT)
            {
                g_sys_state = SYS_FAULT; 
                printf("FAULT TRIGGERED: 0x%08X (DI:0x%02X, I:%.2fA, U:%.2fV, Vce:%.2fV)\r\n", 
                       g_fault_flags, current_di, g_current, g_voltage, g_vce);
            }
        }
        
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1)); 
    }
}
void current_task(void *pvParameters)
{
    pvParameters = pvParameters;

    HC595_Init();
    
    /* 初始状态强制安全输出 0 */
    g_current_ref = 0;
    Current_Output(0);

    while (1)
    {
        /* 1. 等待 TIM6 硬件定时器发出的 1.25ms 同步信号 */
        if (xSemaphoreTake(xSem_CurrentTick, portMAX_DELAY) == pdTRUE)
        {
            g_current_cnt++;

            /* 2. 核心：调用加减流函数 
             * 这个函数里会根据 g_sys_state 是 RUN 还是 STOPPING，
             * 自动对 g_current_ref 进行 +1 或 -1 操作。
             */
            Current_Update(); 

            /* 3. 将最新的 g_current_ref 发送给 595 */
            Current_Output(g_current_ref);
        }
    }
}
void state_task(void *pvParameters)
{
    TickType_t last = xTaskGetTickCount();
    uint8_t cmd; 
    static uint8_t fault_handled = 0;   

    while (1)
    {
        /* 1. 每 5ms 检查一次队列，接收屏幕下发的命令 */
        if (xQueueReceive(g_cmd_queue, &cmd, 0) == pdTRUE)
        {
            switch (cmd)
            {
            case CMD_START:  
                /* ★ 只有自检无故障进入 SYS_IDLE 后，才响应启动指令；
                 *   若自检异常停留在 SYS_FAULT，点击启动无效（禁止启动）
                 */
                if (g_sys_state == SYS_IDLE && g_fault_flags == FAULT_NONE) {
                    g_sys_state = SYS_STARTING; 
                    printf("Button: START Pressed!\r\n");
                } else {
                    printf("START Ignored! System in FAULT (0x%08X).\r\n", g_fault_flags);
                }
                break;

            case CMD_STOP:   
                if (g_sys_state == SYS_RUN || g_sys_state == SYS_STARTING) {
                    g_sys_state = SYS_STOPPING; 
                    printf("Button: STOP Pressed!\r\n");
                }
                break;

            case CMD_RESET:
                printf("Button: RESET Pressed!\r\n");
                if (g_sys_state == SYS_FAULT || g_fault_flags != FAULT_NONE) {
                    g_sys_state = SYS_RESET;
                }
                break;
            }
        }

        /* 2. 状态机主体 */
        switch (g_sys_state)
        {
        case SYS_INIT:
            printf("State: SYS_INIT (Self-testing...)\r\n");
            vTaskDelay(pdMS_TO_TICKS(100)); /* 等待 100ms 让 74HC165 和 ADC 完成上电检测 */
            
            if (g_fault_flags != FAULT_NONE) 
            {
                /* 自检异常：直接进入 SYS_FAULT 禁止启动，用户可去故障页查看具体红字故障 */
                printf("Self-test FAILED! Faults: 0x%08X. Entering FAULT.\r\n", g_fault_flags);
                g_sys_state = SYS_FAULT; 
            } 
            else 
            {
                /* 自检无故障：进入待机状态，等待启动指令 */
                printf("Self-test PASSED. Waiting for START button...\r\n");
                g_sys_state = SYS_IDLE;  
            }
            break;

        case SYS_IDLE:
            break;

        case SYS_STARTING:
            printf("State: SYS_STARTING -> SYS_RUN (Current ramping up)\r\n");
            g_sys_state = SYS_RUN; 
            break;

        case SYS_RUN:
            break;

        case SYS_STOPPING:
            if (g_current_ref == 0)
            {
                printf("Shutdown Complete. State: IDLE\r\n");
                g_sys_state = SYS_IDLE;
            }
            break;

        case SYS_FAULT:
            if (fault_handled == 0) 
            {
                printf("State: SYS_FAULT (Power START Prohibited / Current dropping to 0)\r\n");
                fault_handled = 1;
            }
            break;

        case SYS_RESET:
            printf("Processing Reset Command...\r\n");
            g_fault_flags = FAULT_NONE; 
            vTaskDelay(pdMS_TO_TICKS(30)); 
            
            if (g_fault_flags == FAULT_NONE)
            {
                printf("Reset Success!\r\n");
                HMI_Request_Reset_Tip(1);
                g_sys_state = SYS_IDLE; 
                fault_handled = 0; 
            }
            else
            {
                printf("Reset Failed! Active Faults: 0x%08X\r\n", g_fault_flags);
                HMI_Request_Reset_Tip(0);
                g_sys_state = SYS_FAULT; 
            }
            xQueueReset(g_cmd_queue);
            break;
        }

        vTaskDelayUntil(&last, pdMS_TO_TICKS(5));
    }
}

/* ADC 采集：10ms */
void adc_task(void *pvParameters)
{
    TickType_t last = xTaskGetTickCount();

    /* 启动 ADC */
    ADC_Init();

    while (1)
    {
        /* 把原始值转成实际物理量 */
        float v_i  = ADC_ToVoltage(g_adc_raw[0]);   /* 电流传感器电压 */
        float v_u  = ADC_ToVoltage(g_adc_raw[1]);   /* 电压传感器电压 */
        float v_vc = ADC_ToVoltage(g_adc_raw[2]);   /* 管压降传感器电压 */

        /* 乘上变比得到实际值 */
        g_current = v_i  * ADC_CURRENT_SCALE;
        g_voltage = v_u  * ADC_VOLTAGE_SCALE;
        g_vce     = v_vc * ADC_VCE_SCALE;

        vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
    }
}

/* RS485 Modbus：10ms */
void rs485_task(void *pvParameters)
{
    TickType_t last = xTaskGetTickCount();
    while (1)
    {
        /* TODO: Modbus轮询英杰模块 */
        /* TODO: 根据状态机指令写英杰模块寄存器 */

        vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
    }
}

/* 显示任务：10ms 周期 */
void display_task(void *pvParameters)
{
    pvParameters = pvParameters;
    TickType_t last = xTaskGetTickCount();
    uint16_t update_cnt = 0; 
    
    static uint32_t last_fault_flags = 0xFFFFFFFF; 

    vTaskDelay(pdMS_TO_TICKS(100));
    HMI_Init();

    while (1)
    {
        /* 1. 处理屏幕按键 */
        HMI_Process_Cmd();

        if (g_sys_state != SYS_RESET)
        {
            /* 2. 故障码发生变化时刷新 6 个标签 */
            if (last_fault_flags != g_fault_flags)
            {
                HMI_Update_Fault_Page(g_fault_flags);
                last_fault_flags = g_fault_flags;
            }

            /* 3. 处理复位按钮按下的结果提示与强制同步 */
            if (HMI_Check_Reset_Tip() != 0)
            {
                /* 只要刚刚执行过一次复位提示，同步更新记忆值 */
                last_fault_flags = g_fault_flags;
            }
        }

        /* 4. 每 500ms 刷新电压电流 */
        update_cnt++;
        if (update_cnt >= 50) 
        {
            HMI_Update_VI(g_voltage, g_current);
            update_cnt = 0; 
        }

        vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
    }
}