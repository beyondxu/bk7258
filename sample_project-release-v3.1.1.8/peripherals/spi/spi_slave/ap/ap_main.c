#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>
#include <driver/spi.h>
#include <driver/dma.h>

#define TAG "spi_slave"
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
 * @brief 初始化SPI从设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_init()
{
    spi_config_t config = {0};
    config.role = SPI_ROLE_SLAVE;                       // 角色：主设备
    config.bit_width = SPI_BIT_WIDTH_8BITS;             // 数据位宽：8位
    config.polarity = SPI_POLARITY_HIGH;                // 时钟极性：高电平有效
    config.phase = SPI_PHASE_2ND_EDGE;                  // 时钟相位：第二个时钟沿
    config.wire_mode = SPI_4WIRE_MODE;                  // 4线模式
    config.baud_rate = SPI_BAUD_RATE;                   // 波特率：10MHz
    config.bit_order = SPI_MSB_FIRST;                   // 数据位序：MSB优先
#if (CONFIG_SPI_BYTE_INTERVAL)
    config.byte_interval = 1;                           // 字节间隔：1个时钟周期
#endif
    config.dma_mode = SPI_DMA_MODE_DISABLE;             // DMA模式禁用
    bk_spi_init(SPI_CHAN_ID, &config);
    LOGI("spi init, spi_chan_id=%d\n", SPI_CHAN_ID);
    return BK_OK;
}

/**
 * @brief 反初始化SPI从设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_deinit()
{
    return bk_spi_deinit(SPI_CHAN_ID);
}

/**
 * @brief 初始化SPI从设备的DMA模式
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_dma_init()
{
    spi_config_t config = {0};
    config.role = SPI_ROLE_SLAVE;                       // 角色：主设备
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
    config.dma_mode = SPI_DMA_MODE_ENABLE;              // DMA模式启用
    spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0);      // 分配GSPI0的DMA通道用于发送
    spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0_RX);   // 分配GSPI0_RX的DMA通道用于接收
    config.spi_tx_dma_chan = spi_tx_dma_chan;           // 设置发送DMA通道
    config.spi_rx_dma_chan = spi_rx_dma_chan;           // 设置接收DMA通道
    config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;    // 发送DMA数据宽度：8位
    config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;    // 接收DMA数据宽度：8位
#endif

    bk_spi_init(SPI_CHAN_ID, &config);
    LOGI("spi dma init, spi_chan_id=%d\n", SPI_CHAN_ID);
    return BK_OK;
}

/**
 * @brief 反初始化SPI从设备的DMA模式
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_dma_deinit()
{
    if(spi_tx_dma_chan != DMA_ID_MAX)
    {
        bk_dma_free(DMA_DEV_GSPI0, spi_tx_dma_chan);
    }
    if(spi_rx_dma_chan != DMA_ID_MAX)
    {
        bk_dma_free(DMA_DEV_GSPI0_RX, spi_rx_dma_chan);
    }
    bk_spi_deinit(SPI_CHAN_ID);
    return BK_OK;
}

/**
 * @brief SPI从设备发送数据到主设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_send_data()
{
    uint8_t example_send_buf[SPI_EXAMPLE_BUF_SIZE] = {0};
    for (int i = 0; i < SPI_EXAMPLE_BUF_SIZE; i++)
    {
        example_send_buf[i] = i & 0xff;
    }
    int ret = bk_spi_write_bytes(SPI_CHAN_ID, example_send_buf, SPI_EXAMPLE_BUF_SIZE);
    if(ret != BK_OK)
    {
        LOGE("spi send data failed, ret=%d\n", ret);
        return ret;
    }
    LOGI("spi send data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

/**
 * @brief SPI从设备接收主设备发送的数据
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_recv_data()
{
    uint8_t example_recv_buf[SPI_EXAMPLE_BUF_SIZE] = {0};

    int ret = bk_spi_read_bytes(SPI_CHAN_ID, example_recv_buf, SPI_EXAMPLE_BUF_SIZE);
    if(ret != BK_OK)
    {
        LOGE("spi recv data failed, ret=%d\n", ret);
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
 * @brief SPI从设备进行双向数据传输
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_transmit_data()
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
        LOGE("spi transmit data failed, ret=%d\n", ret);
        return ret;
    }
    for (int i = 0; i < buf_len; i++) {
        LOGI("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
    }
    LOGI("spi transmit data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

/**
 * @brief SPI从设备通过DMA发送数据到主设备
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_dma_send_data()
{
    uint8_t example_send_buf[SPI_EXAMPLE_BUF_SIZE] = {0};
    for (int i = 0; i < SPI_EXAMPLE_BUF_SIZE; i++)
    {
        example_send_buf[i] = i & 0xff;
    }
    int ret = bk_spi_dma_write_bytes(SPI_CHAN_ID, example_send_buf, SPI_EXAMPLE_BUF_SIZE);
    if(ret != BK_OK)
    {
        LOGE("spi dma send data failed, ret=%d\n", ret);
        return ret;
    }
    LOGI("spi dma send data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

/**
 * @brief SPI从设备通过DMA接收主设备发送的数据
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_dma_recv_data()
{
    uint8_t example_recv_buf[SPI_EXAMPLE_BUF_SIZE] = {0};

    int ret = bk_spi_dma_read_bytes(SPI_CHAN_ID, example_recv_buf, SPI_EXAMPLE_BUF_SIZE);
    if(ret != BK_OK)
    {
        LOGE("spi dma recv data failed, ret=%d\n", ret);
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
 * @brief SPI从设备通过DMA进行双向数据传输
 * @return int BK_OK表示成功，其他值表示失败
 */
static int spi_slave_dma_transmit_data()
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
        LOGE("spi dma transmit data failed, ret=%d\n", ret);
        return ret;
    }
    for (int i = 0; i < buf_len; i++) {
        LOGI("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
    }
    bk_spi_dma_duplex_deinit(SPI_CHAN_ID);
    LOGI("spi dma transmit data success, data len=%d\n", SPI_EXAMPLE_BUF_SIZE);
    return BK_OK;
}

static void spi_example_task(beken_thread_arg_t arg)
{
    (void)arg;
    int ret = BK_OK;
    rtos_delay_milliseconds(3000);
    LOGI("spi_example_task start\n");

    //配置普通SPI模式
    ret = spi_slave_init();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }

    //单工接收，slave 先配置好接收，等待master 发送数据
    rtos_delay_milliseconds(1000);
    ret = spi_slave_recv_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }

    //单工发送，slave 先配置好发送，等待master 接受数据
    rtos_delay_milliseconds(1000);
    ret = spi_slave_send_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }

#if 0
    //目前sdk有问题
    //slave 先配置好发送/接受，等待master 接受/发送数据
    rtos_delay_milliseconds(1000);
    ret = spi_slave_transmit_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
#endif
    ret = spi_slave_deinit();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
    rtos_delay_milliseconds(200);

    //配置SPI DMA模式
    ret = spi_slave_dma_init();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }

    //单工接收，slave 先配置好接收，等待master 发送数据
    rtos_delay_milliseconds(1000);
    ret = spi_slave_dma_recv_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
    //单工发送，slave 先配置好发送，等待master 接受数据
    rtos_delay_milliseconds(1000);
    ret = spi_slave_dma_send_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
    //双工，slave 先配置好发送/接受，等待master 接受/发送数据
    rtos_delay_milliseconds(1000);
    ret = spi_slave_dma_transmit_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
    ret = spi_slave_dma_deinit();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }

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
