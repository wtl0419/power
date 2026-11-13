/**
 ****************************************************************************************************
 * @file        lwip_demo
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-04
 * @brief       lwIP SOCKET TCPClient 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */
 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>
#include <stdio.h>
#include <lwip/sockets.h>
#include "./BSP/LCD/lcd.h"
#include "./MALLOC/malloc.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip_demo.h"

#include "lwip/sockets.h"
#include "lwip/api.h"
#include <string.h>
#include "app_global.h"
#define TCP_SERVER_PORT 30000

/* 以太网通信任务：TCP Server */
void eth_task(void *pvParameters)
{
    int server_sock = -1;
    int client_sock = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char rx_buffer[128];
    int recv_len;

    /* 等待 LwIP 底层与硬件 PHY 初始化及自动协商稳定 */
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1)
    {
        /* 1. 创建 TCP Socket */
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock < 0)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        /* 2. 绑定本地 IP (任意网卡) 和端口 (30000) */
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TCP_SERVER_PORT);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            closesocket(server_sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        /* 3. 开始监听，最大排队数量 2 */
        if (listen(server_sock, 2) < 0)
        {
            closesocket(server_sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        while (1)
        {
            /* 4. 阻塞等待上位机连接 (此处不占 CPU) */
            client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_sock < 0) break;

            strcpy(g_net_rx_str, "Client Connected!");

            int err_cnt = 0; /* 连续错误计数器 */

            /* 5. 循环接收上位机数据 */
            while (1)
            {
                memset(rx_buffer, 0, sizeof(rx_buffer));
                /* 阻塞接收客户端数据 */
                recv_len = recv(client_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

                if (recv_len > 0)
                {
                    err_cnt = 0; /* 收到正常数据，清零错误计数 */
                    rx_buffer[recv_len] = '\0'; 
                    
                    /* 将收到的网络数据拷贝给 LCD 显示专用的全局缓存 */
                    snprintf(g_net_rx_str, sizeof(g_net_rx_str), "%s", rx_buffer);
                    
                    /* 回传 ACK 确认 */
                    send(client_sock, "ACK\r\n", 5, 0);
                }
                else if (recv_len == 0)
                {
                    /* recv 返回 0 明确表示客户端主动点击了断开连接 */
                    strcpy(g_net_rx_str, "Client Disconnect");
                    break;
                }
                else 
                {
                    /* recv 返回负数 (-1)。
                       这通常是因为底层 LwIP 触发了接收超时（比如几秒没发数据）。
                       工业级做法：不要立刻断开，而是进行容错计数。 */
                    err_cnt++;
                    if (err_cnt > 20) /* 连续超时/异常达到 20 次，才判定为死链接并踢掉客户端 */
                    {
                        strcpy(g_net_rx_str, "Link Dead");
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100)); /* 稍微延时后继续尝试接收 */
                }
            }
            /* 回收断开的客户端资源 */
            closesocket(client_sock);
            client_sock = -1;
        }
        closesocket(server_sock);
    }
}