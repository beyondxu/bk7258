#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <driver/i2c.h>

#define TAG "i2c_master"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#define I2C_SLAVE_ADDR          0x5A
#define I2C_ID                  I2C_ID_0
#define I2C_EXAMPLE_DATA_SIZE   255
#define I2C_READ_TIMEOUT_MS     1000
#define I2C_WRITE_TIMEOUT_MS    1000

static beken_thread_t i2c_master_thread = NULL;

static int i2c_master_init()
{
    int ret = BK_OK;
    // 初始化I2C驱动
    bk_i2c_driver_init();
    // 初始化I2C配置
    i2c_config_t i2c_cfg = {0};
    i2c_cfg.baud_rate = 400000;
    i2c_cfg.addr_mode = I2C_ADDR_MODE_7BIT;
    i2c_cfg.slave_addr = I2C_SLAVE_ADDR;
    ret = bk_i2c_init(I2C_ID, &i2c_cfg);
    if (ret != BK_OK) {
        LOGE("i2c(%d) init failed, ret = %d\n", I2C_ID, ret);
        return ret;
    }
    LOGI("i2c(%d) init\n", I2C_ID);
    return ret;
}

static int i2c_master_example_send()
{
    int ret = BK_OK;
    uint8_t data_buf[I2C_EXAMPLE_DATA_SIZE] = {0};
    for (uint32_t i = 0; i < I2C_EXAMPLE_DATA_SIZE; i++)
    {
        data_buf[i] = i & 0xff;
    }
    ret = bk_i2c_master_write(I2C_ID, I2C_SLAVE_ADDR, data_buf, I2C_EXAMPLE_DATA_SIZE, I2C_WRITE_TIMEOUT_MS);
    if (ret != BK_OK) {
        LOGE("i2c(%d) master write failed, ret = %d\n", I2C_ID, ret);
        return ret;
    }
    return ret;
}

static int i2c_master_example_recv()
{
    int ret = BK_OK;
    uint8_t recv_buf[I2C_EXAMPLE_DATA_SIZE] = {0};
    ret = bk_i2c_master_read(I2C_ID, I2C_SLAVE_ADDR, recv_buf, I2C_EXAMPLE_DATA_SIZE, I2C_READ_TIMEOUT_MS);
    if (ret != BK_OK) {
        LOGE("i2c(%d) master read failed, ret = %d\n", I2C_ID, ret);
        return ret;
    }
    for (uint32_t i = 0; i < I2C_EXAMPLE_DATA_SIZE; i++) {
        LOGI("i2c_master read 0x%x,\n", recv_buf[i]);
    }
    return ret;
}

static void i2c_master_task(beken_thread_arg_t arg)
{
    (void) arg;
    int ret = BK_OK;

    ret = i2c_master_init();
    if (ret != BK_OK) {
        LOGE("i2c_master_init failed, ret = %d\n", ret);
        TEST_FAIL();
        goto error;
    }
    //等待 slave 准备就绪完成
    rtos_delay_milliseconds(2000);
    ret = i2c_master_example_send();
    if (ret != BK_OK) {
        LOGE("i2c_master_example_send failed, ret = %d\n", ret);
        TEST_FAIL();
        goto error;
    }

    rtos_delay_milliseconds(500);

    ret = i2c_master_example_recv();
    if (ret != BK_OK) {
        LOGE("i2c_master_example_recv failed, ret = %d\n", ret);
        TEST_FAIL();
        goto error;
    }

    TEST_PASS();
error:
    i2c_master_thread = NULL;
    rtos_delete_thread(NULL);
}

int main(void)
{
    bk_init();
    media_service_init();

    if(1/*0 == ate_is_enabled()*/)   //非ATE模式才启动应用代码
    {
        int ret = BK_OK;
#if (I2C_ID == I2C_ID_1)
        // 使用 v3demo 板的 I2C1 需要拉高 GPIO_28，让 SCL 和 SDA 配置为上拉。具体查看原理图。
        gpio_dev_unmap(GPIO_28);
        gpio_config_t mode = {0};
		mode.io_mode = GPIO_OUTPUT_ENABLE;
		mode.func_mode = GPIO_SECOND_FUNC_DISABLE;
		mode.pull_mode = GPIO_PULL_DISABLE;
		bk_gpio_set_config(GPIO_28, &mode);
        bk_gpio_set_output_high(GPIO_28);
#endif
        ret = rtos_create_thread(&i2c_master_thread, 
                                6,
                                "i2c_master_task",
                                i2c_master_task,
                                2048,
                                NULL);
        if (ret != BK_OK)
        {
            LOGE("rtos_create_thread create failed, ret = %d\n", ret);
            TEST_FAIL();
        }
        else
        {
            LOGI("rtos_create_thread create success\r]\n");
        }
    }
    return 0;
}
