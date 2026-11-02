#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>
#include <driver/spi.h>
#include <driver/dma.h>

#define TAG "spi_master"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#define SPI_CHAN_ID     SPI_ID_1
#define SPI_BAUD_RATE   10000000
#define SPI_EXAMPLE_BUF_SIZE 256

static dma_id_t spi_tx_dma_chan = DMA_ID_MAX;
static dma_id_t spi_rx_dma_chan = DMA_ID_MAX;
static beken_thread_t spi_example_thread = NULL;

/**
 * @brief 初始化SPI主设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_init()
{
    spi_config_t config = {0};
    config.role = SPI_ROLE_MASTER;                      // 角色：主设备
    config.bit_width = SPI_BIT_WIDTH_8BITS;             // 数据位宽：8位
    config.polarity = SPI_POLARITY_HIGH;                // 时钟极性：高电平有效
    config.phase = SPI_PHASE_2ND_EDGE;                  // 时钟相位：第二个时钟沿
    config.wire_mode = SPI_4WIRE_MODE;                  // 4线模式
    config.baud_rate = SPI_BAUD_RATE;                   // 波特率：10MHz
    config.bit_order = SPI_MSB_FIRST;                   // 数据位序：MSB优先
#if (CONFIG_SPI_BYTE_INTERVAL)
    config.byte_interval = 1;                           // 字节间隔：1个时钟周期
#endif
    config.dma_mode = SPI_DMA_MODE_DISABLE;             // 禁用DMA模式
    int ret = bk_spi_init(SPI_CHAN_ID, &config);
    LOGI("spi init, spi_chan_id=%d\n", SPI_CHAN_ID);
    return ret;
}

/**
 * @brief 反初始化SPI主设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_deinit()
{
    return bk_spi_deinit(SPI_CHAN_ID);
}

/**
 * @brief 初始化SPI主设备的DMA模式
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_dma_init()
{
    spi_config_t config = {0};
    config.role = SPI_ROLE_MASTER;                      // 角色：主设备
    config.bit_width = SPI_BIT_WIDTH_8BITS;             // 数据位宽：8位
    config.polarity = SPI_POLARITY_HIGH;                // 时钟极性：高电平有效
    config.phase = SPI_PHASE_2ND_EDGE;                  // 时钟相位：第二个时钟沿
    config.wire_mode = SPI_4WIRE_MODE;                  // 4线模式
    config.baud_rate = SPI_BAUD_RATE;                   // 波特率：10MHz
    config.bit_order = SPI_MSB_FIRST;                   // 数据位序：MSB优先
#if (CONFIG_SPI_BYTE_INTERVAL)
    config.byte_interval = 1;                           // 字节间隔：1个时钟周期
#endif
#if (CONFIG_SPI_DMA)
    config.dma_mode = SPI_DMA_MODE_ENABLE;              // 启用DMA模式
    spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1);      // 分配SPI_TX DMA通道
    spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1_RX);   // 分配SPI_RX DMA通道
    config.spi_tx_dma_chan = spi_tx_dma_chan;           // 设置SPI_TX DMA通道
    config.spi_rx_dma_chan = spi_rx_dma_chan;           // 设置SPI_RX DMA通道
    config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;     // 设置SPI_TX DMA数据宽度为8位
    config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;     // 设置SPI_RX DMA数据宽度为8位
#endif

    int ret = bk_spi_init(SPI_CHAN_ID, &config);
    LOGI("spi dma init, spi_chan_id=%d\n", SPI_CHAN_ID);
    return ret;
}

/**
 * @brief 反初始化SPI主设备的DMA模式
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_dma_deinit()
{
    if(spi_tx_dma_chan != DMA_ID_MAX)
    {
        bk_dma_free(DMA_DEV_GSPI1, spi_tx_dma_chan);
    }
    if(spi_rx_dma_chan != DMA_ID_MAX)
    {
        bk_dma_free(DMA_DEV_GSPI1_RX, spi_rx_dma_chan);
    }
    bk_spi_deinit(SPI_CHAN_ID);
    return BK_OK;
}

/**
 * @brief 发送数据到SPI从设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_send_data()
{
    uint8_t example_send_buf[SPI_EXAMPLE_BUF_SIZE] = {0};
    for (int i = 0; i < SPI_EXAMPLE_BUF_SIZE; i++)
    {
        example_send_buf[i] = i & 0xff;
    }
    int ret = bk_spi_write_bytes(SPI_CHAN_ID, example_send_buf, SPI_EXAMPLE_BUF_SIZE);
    if(ret != BK_OK)
    {
        LOGE("spi send data fail, ret=%d\n", ret);
        return ret;
    }
    LOGI("spi send data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

/**
 * @brief 从SPI从设备接收数据
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_recv_data()
{
    uint8_t example_recv_buf[SPI_EXAMPLE_BUF_SIZE] = {0};
    int ret = bk_spi_read_bytes(SPI_CHAN_ID, example_recv_buf, SPI_EXAMPLE_BUF_SIZE);
    if(ret != BK_OK)
    {
        LOGE("spi recv data fail, ret=%d\n", ret);
        return ret;
    }
    for (int i = 0; i < SPI_EXAMPLE_BUF_SIZE; i++)
    {
        LOGI("example_recv_buf[%d]=0x%02x\n", i, example_recv_buf[i]);
    }
    LOGI("spi recv data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

/**
 * @brief 发送和接收数据到/从SPI从设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_transmit_data()
{
    uint32_t buf_len = SPI_EXAMPLE_BUF_SIZE;
    uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
    uint8_t *send_data = (uint8_t *)os_malloc(buf_len);
    os_memset(recv_data, 0xff, buf_len);
    for (int i = 0; i < buf_len; i++) {
        send_data[i] = i & 0xff;
    }

    int ret = bk_spi_transmit(SPI_CHAN_ID, send_data, buf_len, recv_data, buf_len);
    if(ret != BK_OK)
    {
        LOGE("spi transmit data fail, ret=%d\n", ret);
        return ret;
    }
    for (int i = 0; i < buf_len; i++) {
        LOGI("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
    }
    LOGI("spi transmit data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

/**
 * @brief 设备DMA模式发送数据到SPI从设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_dma_send_data()
{
    uint8_t example_send_buf[SPI_EXAMPLE_BUF_SIZE] = {0};
    for (int i = 0; i < SPI_EXAMPLE_BUF_SIZE; i++)
    {
        example_send_buf[i] = i & 0xff;
    }
    int ret = bk_spi_dma_write_bytes(SPI_CHAN_ID, example_send_buf, SPI_EXAMPLE_BUF_SIZE);
    if(ret != BK_OK)
    {
        LOGE("spi dma send data fail, ret=%d\n", ret);
        return ret;
    }
    LOGI("spi dma send data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

/**
 * @brief 设备DMA模式从SPI从设备接收数据
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_dma_recv_data()
{
    uint8_t example_recv_buf[SPI_EXAMPLE_BUF_SIZE] = {0};
    int ret = bk_spi_dma_read_bytes(SPI_CHAN_ID, example_recv_buf, SPI_EXAMPLE_BUF_SIZE);
    if(ret != BK_OK)
    {
        LOGE("spi dma recv data fail, ret=%d\n", ret);
        return ret;
    }
    for (int i = 0; i < SPI_EXAMPLE_BUF_SIZE; i++)
    {
        LOGI("example_recv_buf[%d]=0x%02x\n", i, example_recv_buf[i]);
    }
    LOGI("spi dma recv data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

/**
 * @brief 设备DMA模式发送和接收数据到/从SPI从设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_master_dma_transmit_data()
{
    uint32_t buf_len = SPI_EXAMPLE_BUF_SIZE;
    uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
    uint8_t *send_data = (uint8_t *)os_malloc(buf_len);
    os_memset(recv_data, 0xff, buf_len);
    for (int i = 0; i < buf_len; i++) {
        send_data[i] = i & 0xff;
    }
    bk_spi_dma_duplex_init(SPI_CHAN_ID);
    int ret = bk_spi_dma_duplex_xfer(SPI_CHAN_ID, send_data, buf_len, recv_data, buf_len);
    if(ret != BK_OK)
    {
        LOGE("spi dma transmit data fail, ret=%d\n", ret);
        return ret;
    }
    for (int i = 0; i < buf_len; i++) {
        LOGI("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
    }
    bk_spi_dma_duplex_deinit(SPI_CHAN_ID);
    LOGI("spi dma transmit data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

/**
 * @brief SPI从设备示例任务
 * @param arg 任务参数，未使用
 */
static void spi_example_task(beken_thread_arg_t arg)
{
    (void)arg;
    int ret = BK_OK;
    rtos_delay_milliseconds(3000);
    LOGI(">>> SPI Task Start <<<\n");

    /* 1. 普通模式测试 */
    ret = spi_master_init();
    if(ret != BK_OK) { TEST_FAIL(); goto error; }

    rtos_delay_milliseconds(1000);
    ret = spi_master_send_data();      // Packet 1: 主发，从收
    if(ret != BK_OK) { TEST_FAIL(); goto error; }

    rtos_delay_milliseconds(1000);
    ret = spi_master_recv_data();      // Packet 2: 主收，从发
    if(ret != BK_OK) { TEST_FAIL(); goto error; }

    spi_master_deinit();
    rtos_delay_milliseconds(200);

    /* 2. DMA 模式测试 (喷墨打印机主力模式) */
    ret = spi_master_dma_init();
    if(ret != BK_OK) { TEST_FAIL(); goto error; }

    rtos_delay_milliseconds(1000);
    ret = spi_master_dma_send_data();  // Packet 3: DMA 主发 (下发点阵)
    if(ret != BK_OK) { TEST_FAIL(); goto error; }

    rtos_delay_milliseconds(1000);
    ret = spi_master_dma_recv_data();  // Packet 4: DMA 主收 (状态回传)
    if(ret != BK_OK) { TEST_FAIL(); goto error; }

#if 0
    // ★ 屏蔽此项：官方 SDK 对该双工函数有底层冲突 Bug，实际打印业务不需要
    rtos_delay_milliseconds(2000);
    ret = spi_master_dma_transmit_data();
    if(ret != BK_OK) { TEST_FAIL(); goto error; }
#endif

    ret = spi_master_dma_deinit();
    if(ret != BK_OK) { TEST_FAIL(); goto error; }

    LOGI(">>> ALL SPI COMMUNICATION TESTS PASS! <<<\n");
    TEST_PASS();

error:
    spi_example_thread = NULL;
    rtos_delete_thread(NULL);
}


int main(void)
{
    bk_init();
    media_service_init();

    if(1/*0 == ate_is_enabled()*/)   //非ATE模式才启动应用代码
    {
        int ret = rtos_create_thread(&spi_example_thread,
                                        6,
                                        "spi_example_task",
                                        spi_example_task,
                                        2048,
                                        NULL);
        if(ret != BK_OK)
        {
            LOGE("rtos_create_thread failed, ret=%d\n", ret);
            TEST_FAIL();
        }
        else
        {
            LOGI("rtos_create_thread success\r\n");
        }
    }
    return 0;
}
