#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <bk_private/bk_init.h>
#include <modules/pm.h>

#include "dm_gatts.h"
#include "tph_driver.h"
#include "printer_diagnostics.h"
#include "motor_sdk_driver.h"
#include "ble_printer_service.h"

#include <common/bk_include.h>
#include <driver/gpio.h>
#include <driver/spi.h>
#include <driver/dma.h>
#include <stdio.h>
#include <string.h>

#define TAG "main_ap"

#define PIN_POWER_HOLD      GPIO_14   // 电源维持 (P14)
#define PIN_POWER_KEY       GPIO_40   // 开机/多功能按键 (P40)

/* ★ 关键对齐 1：通信包长双方严格一致 (100 字节) ★ */
#define SPI_PACKET_LEN      TPH_COLUMN_BYTES // 100 字节

#define PIN_BK_SYNC_OUT     GPIO_1   // P1 (BK_1): 连到小华 HC32 的 PC13
#define PIN_BK_READY_IN     GPIO_0   // P0 (BK_2): 连到小华 HC32 的 PH2


#define SPI_CHAN_ID         SPI_ID_1  
#define SPI_BAUD_RATE       10000000  // 10MHz

/* ★ 必须声明全局 DMA 通道句柄 ★ */
static dma_id_t spi_tx_dma_chan = DMA_ID_MAX;
static dma_id_t spi_rx_dma_chan = DMA_ID_MAX;

static beken_thread_t s_pwr_key_task = NULL;
static beken_thread_t s_app_main_task = NULL;
static bool s_printer_link_ready = false;
bool printer_link_is_ready(void) { return s_printer_link_ready; }

extern bk_err_t gpio_dev_unmap(gpio_id_t gpio_id);

// ==========================================
// 1. 电源与按键关机管理
// ==========================================
void pwr_early_hold_power(void) {
    gpio_dev_unmap(PIN_POWER_HOLD);
    gpio_config_t cfg = {
        .io_mode   = GPIO_OUTPUT_ENABLE,
        .pull_mode = GPIO_PULL_DISABLE,
        .func_mode = GPIO_SECOND_FUNC_DISABLE
    };
    bk_gpio_set_config(PIN_POWER_HOLD, &cfg);
    GPIO_UP(PIN_POWER_HOLD);
	 bk_motor_boost_power(PRINTER_DATA_AUDIT ? 0 : 1); /* audit never enables boost */
    os_printf("[PWR_MGR] Power Latch LOCKED (GPIO_14 = HIGH)\r\n");
}

void pwr_system_shutdown(void) {
    os_printf("\r\n>>> Starting System Safe Shutdown Sequence... <<<\r\n");
    tph_power_off();
    bk_motor_set_state(1, 0); 
    bk_motor_set_state(0, 0); 

    while (bk_gpio_get_input(PIN_POWER_KEY) == 0) {
        rtos_delay_milliseconds(50);
    }
    rtos_delay_milliseconds(100); 
    GPIO_DOWN(PIN_POWER_HOLD);
    while (1) { rtos_delay_milliseconds(1000); }
}

static void pwr_key_monitor_task(void *arg) {
    gpio_dev_unmap(PIN_POWER_KEY);
    gpio_config_t cfg = {
        .io_mode   = GPIO_INPUT_ENABLE,
        .pull_mode = GPIO_PULL_UP_EN,
        .func_mode = GPIO_SECOND_FUNC_DISABLE
    };
    bk_gpio_set_config(PIN_POWER_KEY, &cfg);

    uint32_t press_duration_ms = 0;
    while (1) {
        rtos_delay_milliseconds(20); 
        if (bk_gpio_get_input(PIN_POWER_KEY) == 0) {
            press_duration_ms += 20;
            if (press_duration_ms >= 2500) {
                os_printf("[PWR_MGR] Long Press Detected -> Shutting Down!\r\n");
                pwr_system_shutdown();
            }
        } else {
            press_duration_ms = 0;
        }
    }
}

void pwr_key_manager_init(void) {
    pwr_early_hold_power();
    if (s_pwr_key_task == NULL) {
        rtos_create_thread(&s_pwr_key_task, 5, "pwr_key_th", pwr_key_monitor_task, 2048, NULL);
    }
}

// ==========================================
// 2. SPI1 主机 + DMA 模式初始化
// ==========================================

static void test_gpio_input_init(gpio_id_t pin) {
    gpio_dev_unmap(pin);
    gpio_config_t cfg = {
        .io_mode   = GPIO_INPUT_ENABLE,
        .pull_mode = GPIO_PULL_DOWN_EN, // 下拉输入
        .func_mode = GPIO_SECOND_FUNC_DISABLE
    };
    bk_gpio_set_config(pin, &cfg);
}
/**
 * @brief ★ 核心修复：将 P1 (GPIO_1) 从 I2C 解绑，并配置为推挽输出 ★
 */
static void test_gpio_output_init(gpio_id_t pin) {
    gpio_dev_unmap(pin); // ★ 释放 I2C1_SDA 硬件占用 ★
    gpio_config_t cfg = {
        .io_mode   = GPIO_OUTPUT_ENABLE,
        .pull_mode = GPIO_PULL_DISABLE,
        .func_mode = GPIO_SECOND_FUNC_DISABLE
    };
    bk_gpio_set_config(pin, &cfg);
    bk_gpio_set_output_low(pin); // 默认初始拉低
}

/**
 * @brief ★ 补齐：完整的 SPI1 Master DMA 模式初始化 ★
 */
static int spi_master_dma_init(void) {
    spi_config_t config = {0};
    config.role         = SPI_ROLE_MASTER;
    config.bit_width    = SPI_BIT_WIDTH_8BITS;
    config.polarity     = SPI_POLARITY_HIGH;            // Mode 3 (CPOL=1)
    config.phase        = SPI_PHASE_2ND_EDGE;          // Mode 3 (CPHA=1)
    config.wire_mode    = SPI_4WIRE_MODE;
    config.baud_rate    = SPI_BAUD_RATE;               // 10MHz
    config.bit_order    = SPI_MSB_FIRST;

    // ★ 关键：分配并启用硬件 DMA 搬运通道 ★
    config.dma_mode = SPI_DMA_MODE_ENABLE;
    spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1);     // 分配 TX DMA 通道
    spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1_RX);  // 分配 RX DMA 通道
    config.spi_tx_dma_chan  = spi_tx_dma_chan;
    config.spi_rx_dma_chan  = spi_rx_dma_chan;
    config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
    config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;

    int ret = bk_spi_init(SPI_CHAN_ID, &config);
    os_printf("[SPI] SPI1 Master DMA Mode Init %s, tx_ch=%d, rx_ch=%d\r\n", 
              (ret == BK_OK) ? "SUCCESS" : "FAIL", spi_tx_dma_chan, spi_rx_dma_chan);
    return ret;
}

// ==========================================
// 3. 开机硬件自检与握手 (使用标准 100 字节 DMA)
// ==========================================

bool bk_spi_master_selftest_100b(void) {
    uint8_t tx_buf[SPI_PACKET_LEN];
    uint8_t rx_buf[SPI_PACKET_LEN];

    for (int i = 0; i < SPI_PACKET_LEN; i++) {
        tx_buf[i] = (uint8_t)(i & 0xFF);
    }
    memset(rx_buf, 0, sizeof(rx_buf));

    os_printf("\r\n[BK7258] >>> Starting Active-LOW Handshake SelfTest... <<<\r\n");

    // 初始状态：P1 (BK_1) 保持高电平 (空闲状态，配合上拉)
    bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
    rtos_delay_milliseconds(10);

    // ====================================================
    // ★ 步骤 1：发送 100 字节测试包
    // ====================================================
    os_printf("[BK7258] Step 1: Requesting HC32 (Pull P1 LOW)...\r\n");
    bk_gpio_set_output_low(PIN_BK_SYNC_OUT); // ★ 主机发起请求：拉低 P1 (0V) ★

    // 等待 HC32 确认就绪：等待 P0 (BK_2) 被拉低响应 (最多 500ms)
    uint32_t timeout = 500;
    while ((bk_gpio_get_input(PIN_BK_READY_IN) == 1) && timeout) {
        rtos_delay_milliseconds(1);
        timeout--;
    }
    if (timeout == 0) {
        os_printf("[BK7258 Error] HC32 no ACK on Step 1 (P0 still HIGH)!\r\n");
        bk_gpio_set_output_high(PIN_BK_SYNC_OUT); // 超时释放为高
        return false;
    }

    // 写入 100 字节
    bk_spi_dma_write_bytes(SPI_CHAN_ID, tx_buf, SPI_PACKET_LEN);

    // 释放请求：恢复 P1 为高电平
    bk_gpio_set_output_high(PIN_BK_SYNC_OUT);

    // 等待 HC32 完成并释放 P0 变高
    timeout = 500;
    while ((bk_gpio_get_input(PIN_BK_READY_IN) == 0) && timeout) {
        rtos_delay_milliseconds(1);
        timeout--;
    }

    // ====================================================
    // ★ 步骤 2：读回 100 字节取反数据
    // ====================================================
    rtos_delay_milliseconds(5);
    os_printf("[BK7258] Step 2: Requesting Read (Pull P1 LOW)...\r\n");
    bk_gpio_set_output_low(PIN_BK_SYNC_OUT); // ★ 主机发起读请求：再次拉低 P1 (0V) ★

    // 等待 HC32 装载好并拉低 P0
    timeout = 500;
    while ((bk_gpio_get_input(PIN_BK_READY_IN) == 1) && timeout) {
        rtos_delay_milliseconds(1);
        timeout--;
    }
    if (timeout == 0) {
        os_printf("[BK7258 Error] HC32 no ACK on Step 2!\r\n");
        bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
        return false;
    }

    // 读取 100 字节
    bk_spi_dma_read_bytes(SPI_CHAN_ID, rx_buf, SPI_PACKET_LEN);

    // 释放请求：恢复高电平
    bk_gpio_set_output_high(PIN_BK_SYNC_OUT);

    // 等待 HC32 释放 P0 变高
    timeout = 500;
    while ((bk_gpio_get_input(PIN_BK_READY_IN) == 0) && timeout) {
        rtos_delay_milliseconds(1);
        timeout--;
    }

    // ====================================================
    // ★ 校验结果
    // ====================================================
    os_printf("TX Sent     : %02X %02X %02X %02X ...\r\n", tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3]);
    os_printf("RX Received : %02X %02X %02X %02X ...\r\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);

    bool pass = true;
    for (int i = 0; i < SPI_PACKET_LEN; i++) {
        if (rx_buf[i] != (uint8_t)(~tx_buf[i])) {
            pass = false;
            break;
        }
    }

    if (pass) {
        os_printf("==============================================\r\n");
        os_printf(">>> [SUCCESS] ACTIVE-LOW HANDSHAKE VERIFIED! <<<\r\n");
        os_printf("==============================================\r\n");
    } else {
        os_printf(">>> [FAILED] SelfTest Echo Mismatch! <<<\r\n");
    }

    return pass;
}


void u5_paper_motor_line_feed_test(void);
void u4_correct_while_loop_test(void) ;
extern void u4_carriage_motor_standalone_test(void);
extern void test_print_solid_color_bar(uint8_t color_mask, uint32_t total_cols);
extern void test_print_multi_lines_for_scope(uint8_t color_mask, uint32_t lines);
extern void test_print_color_bars_multi_lines(uint8_t color_byte, uint32_t total_cols, uint32_t lines); 
extern void test_print_perfect_green_multi_lines(uint32_t print_width_cols, uint32_t total_lines);
extern void test_print_full_cmy_color_palette(uint32_t print_width_cols);
void test_print_standalone_single_black_line(void) ;

// ==========================================
// 4. 应用任务主入口
// ==========================================
static void app_printer_task_entry(void *arg) {
    rtos_delay_milliseconds(500);

    os_printf("\r\n************************************************\r\n");
    os_printf("    BK7258 DN20 Inkjet Printer Starting...      \r\n");
    os_printf("************************************************\r\n");
	rtos_delay_milliseconds(500);
	
	   // ★ 直接运行安全往复测试 ★
	 //  u4_carriage_motor_standalone_test();
	//   return ;

#if PRINTER_CARRIAGE_TEST
    /* Reference test exits before ANY SPI/HC32/BLE printer setup. */
    os_printf("[CARRIAGE_REFERENCE] standalone before_SPI before_HC32 before_BLE_printer\r\n");
    printer_run_carriage_diagnostic();
    rtos_delete_thread(NULL);
    return;
#endif

#if !PRINTER_DATA_AUDIT || PRINTER_BOUNDED_PRINT
    // 1. 初始化电机 GPIO
    bk_motor_gpio_config_init();

   // 2. ★ 核心修复：同时初始化两个握手引脚！★
    test_gpio_output_init(PIN_BK_SYNC_OUT); // P1 (BK_1) -> 配置为推挽输出！
    test_gpio_input_init(PIN_BK_READY_IN);  // P0 (BK_2) -> 配置为下拉输入！
    // 3. ★ 启动硬件 SPI1 DMA 模式 ★
    spi_master_dma_init();
	// ★ 直接运行 U4 字车单测循环 ★
	

    // 4. 开机自检：确认小华固件在线且通信正常
    bool link_ready = false;
    int retry = 0;
    while (retry < 5) {
        retry++;
        if (bk_spi_master_selftest_100b()) {
            link_ready = true;
            break;
        }
        rtos_delay_milliseconds(500);
    }
	// 握手自检成功...
    s_printer_link_ready = link_ready;
    if (!link_ready) {
        os_printf("[SYS] Check HC32 link before printing.\r\n");
    }
	   rtos_delay_milliseconds(1000);
    if (link_ready) printer_run_startup_diagnostic();
	   // ★ 测试打印：绿色 (青+黄, 掩码 0x33, 打印 600 列) ★
	  // test_print_solid_color_bar(COLOR_MASK_GREEN, 600);
	// test_print_multi_lines_for_scope(COLOR_MASK_GREEN, 10);
	   // 如果想测品红 (Magenta)，换成：
	   // test_print_solid_color_bar(COLOR_MASK_MAGENTA, 600);
	
	   // 如果想测三色全喷 (黑色色块)，换成：
	   // test_print_solid_color_bar(COLOR_MASK_ALL_BLACK, 600);
	
	   // 打印完成之后，再进入 BLE 监听服务
	// ★ 测试 1：打印青色 (Cyan, 0x03) 2 行 ★
		os_printf("\r\n>>> TEST 1: CYAN (0x03) <<<\r\n");
	 // ★★★ 在这里调用测试！打印 600 列宽，连续打 4 行 ★★★
      // ★★★ 直接调这行即可：内部自动先霍尔寻原点，再打 6 行纯绿 ★★★
     // ★★★ 在此调用 7 色测试：色块宽度 600 列 ★★★
    //test_print_full_cmy_color_palette(600);
		rtos_delay_milliseconds(1500);

#else
    os_printf("[AUDIT] receive-only; HC32 link test, motor and ink diagnostics disabled\r\n");
#endif

    // 5. 自检通过，启动蓝牙广播与打印机协议后台任务
    dm_gatts_init();
    printer_server_init(); // 内部已包含 hw_printer_create_task

    os_printf("[SYS] System Initialization Finished. Waiting for Phone BLE...\r\n");

    // 退出初始化线程，后续由 prt_worker 线程处理实际打印
    rtos_delete_thread(NULL);
}


int main(void) {
    // 步骤 1：系统基础初始化
    bk_init();

    // 步骤 2：第一时间锁死电源保持脚 GPIO_14
    pwr_early_hold_power();

    // 步骤 3：开启按键关机监控
    pwr_key_manager_init();

    // 步骤 4：主频锁定在 480MHz 保证高速图像解码能力
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_480M);
	

    // 步骤 5：启动打印主流程任务
    rtos_create_thread(&s_app_main_task, 4, "app_task", app_printer_task_entry, 4096, NULL);

    return 0;
}

