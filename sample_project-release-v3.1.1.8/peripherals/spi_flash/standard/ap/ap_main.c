#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>


#if (CONFIG_SPI_MST_FLASH)
#include <driver/spi_flash.h>
#endif
#include "driver/gpio.h"
#include "gpio_driver.h"


#define TAG "main"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#define DEV_LDO_IO GPIO_13       // 设备LDO使能IO
#define DEV_FLASH_WP_IO GPIO_6   // 设备FLASH WP IO
#define DEV_FLASH_RST_IO GPIO_7  // 设备FLASH RST IO
#define DEV_SPI_ID SPI_ID_1      // 设备SPI ID


static int app_spi_flash_test(void)
{
    int ret = BK_OK;
    gpio_config_t mode = {0};
    gpio_id_t gpio_id;

    // 配置LDO IO为输出模式, 初始化 GPIO 输出高电平
    gpio_id = DEV_LDO_IO;
    BK_LOG_ON_ERR(gpio_dev_unmap(gpio_id));
    mode.io_mode = GPIO_OUTPUT_ENABLE;
    mode.pull_mode = GPIO_PULL_DISABLE;
    BK_LOG_ON_ERR(bk_gpio_set_config(gpio_id, &mode));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(gpio_id));

    // 配置FLASH WP IO为输出模式, 初始化 GPIO 输出高电平
    gpio_id = DEV_FLASH_WP_IO;
    BK_LOG_ON_ERR(gpio_dev_unmap(gpio_id));
    mode.io_mode = GPIO_OUTPUT_ENABLE;
    mode.pull_mode = GPIO_PULL_DISABLE;
    BK_LOG_ON_ERR(bk_gpio_set_config(gpio_id, &mode));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(gpio_id));

    // 配置FLASH RST IO为输出模式, 初始化 GPIO 输出高电平
    gpio_id = DEV_FLASH_RST_IO;
    BK_LOG_ON_ERR(gpio_dev_unmap(gpio_id));
    mode.io_mode = GPIO_OUTPUT_ENABLE;
    mode.pull_mode = GPIO_PULL_DISABLE;
    BK_LOG_ON_ERR(bk_gpio_set_config(gpio_id, &mode));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(gpio_id));

    // 等待LDO稳定
    rtos_delay_milliseconds(100);

    // 初始化SPI Flash
    spi_id_t spi_id = DEV_SPI_ID;
    BK_LOG_ON_ERR(bk_spi_flash_init(spi_id));
    LOGI("[%s][%d] Spi flash driver init success, SPI ID: %d\n", __FUNCTION__, __LINE__, spi_id);

    // 读取Flash ID
    uint32_t flash_id = bk_spi_flash_read_id(spi_id);
    LOGI("[%s][%d] Flash ID: 0x%x\n", __FUNCTION__, __LINE__, flash_id);

    // 准备测试数据
    #define TEST_FLASH_ADDR    0x0       // 测试地址
    #define TEST_UNIT_SIZE     256       // 每次操作的单元大小
    #define TOTAL_TEST_SIZE    5000      // 总测试数据大小
    #define NUM_OPERATIONS     ((TOTAL_TEST_SIZE + TEST_UNIT_SIZE - 1) / TEST_UNIT_SIZE) // 计算需要的操作次数
    
    // 准备写入和读取缓冲区
    uint8_t write_data[TEST_UNIT_SIZE] = {0};
    uint8_t read_data[TEST_UNIT_SIZE] = {0};

    // 首先擦除整个测试区域
    LOGI("[%s][%d] Erasing Flash at 0x%x, total size: %d bytes\n", __FUNCTION__, __LINE__, TEST_FLASH_ADDR, TOTAL_TEST_SIZE);
    BK_LOG_ON_ERR(bk_spi_flash_erase(spi_id, TEST_FLASH_ADDR, TOTAL_TEST_SIZE));
    LOGI("[%s][%d] Flash erase success\n", __FUNCTION__, __LINE__);

    // 分多次写入数据
    LOGI("[%s][%d] Writing total %d bytes data to Flash in %d operations\n", __FUNCTION__, __LINE__, TOTAL_TEST_SIZE, NUM_OPERATIONS);
    for (int op = 0; op < NUM_OPERATIONS; op++) 
    {
        uint32_t current_addr = TEST_FLASH_ADDR + op * TEST_UNIT_SIZE;
        uint32_t current_size = (op == NUM_OPERATIONS - 1) ? (TOTAL_TEST_SIZE % TEST_UNIT_SIZE) : TEST_UNIT_SIZE;
        if (current_size == 0) current_size = TEST_UNIT_SIZE;
        
        // 生成当前扇区的测试数据
        for (int i = 0; i < current_size; i++) 
        {
            write_data[i] = op * TEST_UNIT_SIZE + i;
        }
        
        // 写入当前扇区数据
        LOGI("[%s][%d] Writing operation %d/%d: addr=0x%x, size=%d bytes\n", __FUNCTION__, __LINE__, op + 1, NUM_OPERATIONS, current_addr, current_size);
        BK_LOG_ON_ERR(bk_spi_flash_write(spi_id, current_addr, write_data, current_size));
    }
    LOGI("[%s][%d] All Flash write operations completed\n", __FUNCTION__, __LINE__);
    
    // 分多次读取数据并验证
    LOGI("[%s][%d] Reading and verifying total %d bytes data from Flash\n", __FUNCTION__, __LINE__, TOTAL_TEST_SIZE);
    bool data_match = true;
    
    for (int op = 0; op < NUM_OPERATIONS; op++) 
    {
        uint32_t current_addr = TEST_FLASH_ADDR + op * TEST_UNIT_SIZE;
        uint32_t current_size = (op == NUM_OPERATIONS - 1) ? (TOTAL_TEST_SIZE % TEST_UNIT_SIZE) : TEST_UNIT_SIZE;
        if (current_size == 0) current_size = TEST_UNIT_SIZE;
        
        // 读取当前扇区数据
        LOGI("Reading operation %d/%d: addr=0x%x, size=%d bytes\n", op + 1, NUM_OPERATIONS, current_addr, current_size);
        BK_LOG_ON_ERR(bk_spi_flash_read(spi_id, current_addr, read_data, current_size));
        
        // 生成预期数据并比较
        for (int i = 0; i < current_size; i++) 
        {
            uint8_t expected_data = op * TEST_UNIT_SIZE + i;
            if (read_data[i] != expected_data) 
            {
                data_match = false;
                LOGI("Data mismatch at global index %d: expected=0x%x, read=0x%x\n", op * TEST_UNIT_SIZE + i, expected_data, read_data[i]);
                break;
            }
        }
        
        if (!data_match) 
        {
            break;
        }
    }

    // 打印测试结果
    if (data_match) 
    {
        LOGI("[%s][%d] Flash test PASS: All write and read data match!\n", __FUNCTION__, __LINE__);
    } 
    else 
    {
        LOGI("[%s][%d] Flash test FAIL: Data mismatch found!\n", __FUNCTION__, __LINE__);
        ret = BK_FAIL;
    }

    return ret;
}

int main(void)
{
    bk_init();
    media_service_init();

    if (BK_OK == app_spi_flash_test())
    {
        TEST_PASS();
    }
    else
    {
        TEST_FAIL();
    }

    return 0;
}
