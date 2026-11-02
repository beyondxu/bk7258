#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include <driver/uart.h>
#include "dev_uart.h"

// 日志标签定义
#define TAG "uart"

// 日志输出宏定义
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// 测试通过和失败的宏定义
#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

// UART接收信号量，用于同步接收数据
static beken_semaphore_t g_uart_rx_sem = NULL;

/**
 * @brief UART接收中断回调函数
 * @param id UART端口号
 * @param param 传递给中断回调的参数
 * @note 当UART有数据可读时，该函数会被调用，释放信号量通知主程序读取数据
 */
static void _uart_rx_can_read_isr(uart_id_t id, void *param)
{
    if(g_uart_rx_sem)
    {
        rtos_set_semaphore(&g_uart_rx_sem);
    }
}

/**
 * @brief UART发送中断回调函数
 * @param id UART端口号
 * @param param 传递给中断回调的参数
 * @note 当UART可以发送数据时，该函数会被调用
 */
static void _uart_tx_can_send_isr(uart_id_t id, void *param)
{
    LOGE("[%s][%d] uart %d can write data\r\n", __FUNCTION__, __LINE__, id);
}

/**
 * @brief UART测试主函数
 * @return BK_OK 表示成功
 * @note 该函数完成UART的初始化配置，并启动回环测试
 */
static int app_uart_test(void)
{
    bk_err_t ret = BK_FAIL;
    char buff[256] = {0};
    
    // UART配置参数结构体
    uart_config_t config =
    {
            .baud_rate = UART_BAUDRATE_115200,  // 波特率：115200
            .data_bits = UART_DATA_8_BITS,      // 数据位：8位
            .parity = UART_PARITY_NONE,         // 校验位：无
            .stop_bits = UART_STOP_BITS_1,      // 停止位：1位
            .flow_ctrl = UART_FLOWCTRL_DISABLE, // 流控：禁用
            .src_clk = UART_SCLK_XTAL_26M       // 源时钟：26M晶振
    };

    do{
        // 打开UART1设备，配置参数并注册中断回调函数
        ret = dev_uart_open(UART_ID_1, &config, _uart_rx_can_read_isr, _uart_tx_can_send_isr);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] open uart1 fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            TEST_FAIL();
            break;
        }

        // 初始化信号量，用于同步接收数据
        rtos_init_semaphore(&g_uart_rx_sem, 1);
        
        // 发送欢迎消息
        bk_uart_write_bytes(UART_ID_1, "hello, this message is sent from bk7258\r\n", os_strlen("hello, this message is sent from bk7258\r\n"));
        
        // 进入主循环，持续监听串口数据
        while(1)
        {
            // 等待接收信号量，超时时间200ms
            if(BK_OK != rtos_get_semaphore(&g_uart_rx_sem, 200))
            {
                continue;
            }
            
            // 清空接收缓冲区
            os_memset(buff, 0, sizeof(buff));
            
            // 从UART读取数据，超时时间100ms
            ret = bk_uart_read_bytes(UART_ID_1, buff, sizeof(buff), 100);
            LOGI("[%s][%d] uart %d recv:%s\r\n", __FUNCTION__, __LINE__, UART_ID_1, buff);

            // 如果接收到数据，原样返回
            if(ret > 0)
            {
                bk_uart_write_bytes(UART_ID_1, buff, ret);

                if(strstr(buff, "hello, this message is sent from bk7258"))
                {
                    TEST_PASS();
                    break;
                }
                else
                {
                    TEST_FAIL();
                    break;
                }
            }
        }
    }while(0);

    return BK_OK;
}

/**
 * @brief 主函数
 * @return 0
 * @note 程序入口，完成系统初始化并启动UART测试
 */
int main(void)
{
    // 初始化BK系统
    bk_init();
    // 初始化媒体服务
    media_service_init();

    // 启动UART测试
    app_uart_test();
    
    return 0;
}
