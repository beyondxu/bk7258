#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/uart.h>

#define TAG "uart"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/**
 * @brief 打开UART设备
 * @param uart_id UART端口号，指定要打开的UART设备
 * @param config UART配置参数指针，包含波特率、数据位、校验位等配置信息
 * @param rx_isr UART接收中断回调函数指针，当UART有数据可读时会被调用
 * @param tx_isr UART发送中断回调函数指针，当UART可以发送数据时会被调用
 * @return BK_OK 表示成功，其他值表示失败
 * @note 该函数完成UART设备的初始化配置，包括：
 *       1. 检查配置参数有效性
 *       2. 根据配置使能接收/发送DMA
 *       3. 使能接收中断
 *       4. 注册接收和发送中断回调函数
 *       5. 初始化UART设备
 */
s32 dev_uart_open(uart_id_t uart_id, uart_config_t *config, uart_isr_t rx_isr, uart_isr_t tx_isr)
{
    bk_err_t ret = BK_FAIL;

    do{
        // 检查配置参数是否有效
        if(!config)
        {
            LOGE("[%s][%d] config is NULL\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if(config->enable_sw_flow_ctrl)
        {
#if !CONFIG_UART_SW_FLOW_CTRL
            LOGE("[%s][%d] need enable CONFIG_UART_SW_FLOW_CTRL to support sw flow ctrol\r\n", __FUNCTION__, __LINE__);
            break;
#endif
        }

#if CONFIG_UART_RX_DMA
        // 如果配置了UART接收DMA，则使能接收DMA
        config->rx_dma_en = true;                    //使能RX dma
        bk_uart_enable_sw_fifo(uart_id);
#endif

#if CONFIG_UART_TX_DMA
        // 如果配置了UART发送DMA，则使能发送DMA
        config->tx_dma_en = true;                    //使能TX dma
        bk_uart_enable_sw_fifo(uart_id);
#endif

        // 初始化UART设备
        ret = bk_uart_init(uart_id, config);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] uart init fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }

        // 使能UART接收中断
        bk_uart_enable_rx_interrupt(uart_id);
        bk_uart_enable_tx_interrupt(uart_id);
        
        // 注册UART接收中断回调函数
        bk_uart_register_rx_isr(uart_id, rx_isr, NULL);

        // 注册UART发送中断回调函数
        bk_uart_register_tx_isr(uart_id, tx_isr, NULL);
        
        LOGI("[%s][%d] uart %d open success\r\n", __FUNCTION__, __LINE__, uart_id);
    }while(0);

    return ret;
}

/**
 * @brief 关闭UART设备
 * @param uart_id UART端口号
 * @return BK_OK 表示成功，其他值表示失败
 * @note 该函数完成UART设备的反初始化，释放相关资源
 */
s32 dev_uart_close(uart_id_t uart_id)
{
    bk_err_t ret = BK_FAIL;

    do{
        // 反初始化UART设备
        ret = bk_uart_deinit(uart_id);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] uart deinit fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }

        LOGI("[%s][%d] uart %d close success\r\n", __FUNCTION__, __LINE__, uart_id);
    }while(0);

    return ret;
}
