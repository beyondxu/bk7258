#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>
#include "gpio_driver.h"
#include <driver/sim_spi.h>

#define TAG "sim_spi"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

static beken_thread_t sim_spi_example_thread = NULL;
static bk_lcd_spi_handle_t *sim_spi_handle = NULL;

#define SIM_SPI_SCK     GPIO_20
#define SIM_SPI_CS      GPIO_21
#define SIM_SPI_SDA     GPIO_22
#define SIM_SPI_RST     GPIO_23

/**
 * @brief 模拟SPI输出示例
 * @return bk_err_t BK_OK 成功 BK_FAIL 失败
 */
int sim_spi_example(void)
{
    int ret = BK_OK;
    lcd_spi_io_t sim_spi_io = {
        .clk = SIM_SPI_SCK,
        .cs = SIM_SPI_CS,
        .sda = SIM_SPI_SDA,
        .rst = SIM_SPI_RST,
    };

    // 注册模拟SPI总线
    sim_spi_handle = lcd_spi_bus_io_register(&sim_spi_io);
    if(sim_spi_handle == NULL)
    {
        LOGE("sim_spi_bus_io_register fail");
        TEST_FAIL();
        return BK_FAIL;
    }

    // 初始化模拟SPI总线, 使用spi3线模式，时钟极性高电平，相位极性为第二个时钟沿
    ret = sim_spi_handle->init(sim_spi_handle);
    if(ret != BK_OK)
    {
        LOGE("sim_spi_handle->init fail");
        TEST_FAIL();
        return BK_FAIL;
    }
    // 3 线串行数据包 包含控制位 D/CX 和传输字节。一共有 9 bits，第一个bit 为 D/CX，
    // 0 表示命令，1 表示数据。后面 8 bits 为传输字节。

    // 发送数据 0x00 ~ 0xFF
    rtos_delay_milliseconds(100);
    for (int i = 0; i < 255; i++)
    {
        i = i & 0xFF;
        sim_spi_handle->write_data(sim_spi_handle, i);
    }
    LOGI("write_data 0x00 ~ 0xFF done\r\n");
   
    // 发送命令 0xFF ~ 0x00
    rtos_delay_milliseconds(100);
    for (int i = 255; i >= 0; i--)
    {
        i = i & 0xFF;
        sim_spi_handle->write_cmd(sim_spi_handle, i);
    }
    LOGI("write_cmd 0xFF ~ 0x00 done\r\n");

    // 反初始化模拟SPI总线
    ret = sim_spi_handle->deinit(sim_spi_handle);
    if(ret != BK_OK)
    {
        LOGE("sim_spi_handle->deinit fail");
        TEST_FAIL();
        return BK_FAIL;
    }
    TEST_PASS();
    return BK_OK;
}

/**
 * @brief 模拟SPI输出任务
 * @param arg 任务参数
 */
static void sim_spi_example_task(beken_thread_arg_t arg)
{
    (void)arg;
  
    sim_spi_example();

    sim_spi_example_thread = NULL;

    rtos_delete_thread(NULL);
}

int main(void)
{
    bk_init();
    media_service_init();

    if(1/*0 == ate_is_enabled()*/)   //非ATE模式才启动应用代码
    {
        int ret = rtos_create_thread(&sim_spi_example_thread,
                                    6, 
                                    "sim_spi_example_thread",
                                    sim_spi_example_task,
                                    2048,
                                    NULL);
        if(ret != BK_OK)
        {
            LOGE("rtos_create_thread failed, ret:%x\r\n", ret);
            TEST_FAIL();
        }
        else
        {
            LOGI("rtos_create_thread success\r\n");
        }
    }
    return 0;
}
