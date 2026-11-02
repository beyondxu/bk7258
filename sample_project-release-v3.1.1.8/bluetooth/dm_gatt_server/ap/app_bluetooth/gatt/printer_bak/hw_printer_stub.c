#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <components/log.h>
#include <os/os.h>
#include <os/mem.h>

#include "tph_driver.h"
#include "motor_sdk_driver.h"
#include "printer_protocol.h"
#include "ble_printer_service.h"

#define TAG "HW_PRINTER"

// 进纸参数校准：200 喷孔 (8.46mm) 实测无缝拼接微步数
#define LINE_FEED_STEPS        2920   // 行间进纸步数 (消除重叠的核心参数)
#define PAPER_EJECT_STEPS      5000   // 打印完毕出纸步数

static uint8_t *s_print_buffer = NULL;
static uint32_t s_max_buffer_size = 0;
static uint32_t s_buffer_len = 0;

static beken_thread_t s_print_task_handle = NULL;
static beken_semaphore_t s_print_sem = NULL;

// 1180 列打火缓冲区 (每列 C:25B, M:25B, Y:25B, 共 200 个喷孔全开)
static TphTestColumn_t s_decoded_line[1180];
static uint8_t s_unpacked_swath[32768];
__IRAM_SEC void bk_delay_us(UINT32 us);

void hw_printer_init(void)
{
}

bool hw_printer_prepare_buffer(uint32_t total_len)
{
    if (s_print_buffer) {
        psram_free(s_print_buffer);
        s_print_buffer = NULL;
    }
    
    uint32_t alloc_size = total_len + 1024;
    s_print_buffer = (uint8_t *)psram_malloc(alloc_size);

    if (!s_print_buffer) {
        os_printf("[PRN_MEM_ERR] Failed to allocate %lu bytes from PSRAM Heap!\r\n", alloc_size);
        return false;
    }

    s_max_buffer_size = alloc_size;
    s_buffer_len = 0;
    os_printf("[PRN_MEM] Successfully allocated %lu bytes at PSRAM %p\r\n", s_max_buffer_size, s_print_buffer);
    return true;
}

void hw_printer_append_data(const uint8_t *data, uint16_t len)
{
    if (s_print_buffer != NULL && (s_buffer_len + len <= s_max_buffer_size)) {
        memcpy(s_print_buffer + s_buffer_len, data, len);
        s_buffer_len += len;
    }
}

void hw_printer_clear_buffer(void)
{
    if (s_print_buffer) {
        psram_free(s_print_buffer);
        s_print_buffer = NULL;
    }
    s_buffer_len = 0;
    s_max_buffer_size = 0;
}

/* 标准 PackBits 解包 */
static int decode_packbits_full(const uint8_t *src, int src_len, uint8_t *dst, int max_dst_len)
{
    int src_idx = 0;
    int dst_idx = 0;

    while (src_idx < src_len && dst_idx < max_dst_len) {
        uint8_t flag = src[src_idx++];

        if (flag == 0x80) {
            continue;
        } else if (flag > 0x80) {
            int count = 256 - flag + 1;
            if (src_idx >= src_len) break;
            uint8_t val = src[src_idx++];
            for (int i = 0; i < count && dst_idx < max_dst_len; i++) {
                dst[dst_idx++] = val;
            }
        } else {
            int count = flag + 1;
            for (int i = 0; i < count && src_idx < src_len && dst_idx < max_dst_len; i++) {
                dst[dst_idx++] = src[src_idx++];
            }
        }
    }
    return dst_idx;
}

/* ★ 精确平稳进纸函数：解决进纸过小和卡纸重叠 ★ */
static void feed_paper_calibrated(uint32_t microsteps)
{
    GPIO_DOWN(PAPER_MOT_DIR);
    bk_delay_us(20);

    for (uint32_t i = 0; i < microsteps; i++) {
        GPIO_UP(PAPER_MOT_STEP);
        bk_delay_us(4);
        GPIO_DOWN(PAPER_MOT_STEP);
        bk_delay_us(250); 
    }
}

/* 字车快速复位归位 */
static void carriage_return_home_fast(void)
{
    GPIO_DOWN(TPH_MOT_DIR);
    bk_delay_us(20);

    for (uint32_t i = 0; i < 7480; i++) {
        uint32_t delay = 200; 
        if (i < 400) delay = 800 - (600 * i / 400);
        else if (i > 7080) delay = 200 + (600 * (i - 7080) / 400);

        GPIO_UP(TPH_MOT_STEP);
        bk_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        bk_delay_us(delay);
    }
}

/* 字车打火扫掠 */
static void sweep_and_fire_line(const TphTestColumn_t *line_data)
{
    GPIO_UP(TPH_MOT_DIR);
    bk_delay_us(20);

    for (int step = 0; step < 200; step++) {
        uint32_t delay = 1000 - ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        bk_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        bk_delay_us(delay);
    }

    for (uint32_t col = 0; col < 1180; col++) {
        tph_fire_one_column(&line_data[col]);

        for (int m = 0; m < 6; m++) {
            GPIO_UP(TPH_MOT_STEP);
            bk_delay_us(4);
            GPIO_DOWN(TPH_MOT_STEP);
            bk_delay_us(300); 
        }
    }

    for (int step = 0; step < 200; step++) {
        uint32_t delay = 300 + ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        bk_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        bk_delay_us(delay);
    }
}

/* 主打印任务调度 */
static void parse_and_execute_stream_perfect(const uint8_t *buf, uint32_t len)
{
    os_printf("\r\n========================================================\r\n");
    os_printf(">>> Starting Printing with Calibrated Feed (Len=%lu) <<<\r\n", len);
    os_printf("========================================================\r\n");

    // 硬件供电与电机初始化 (任务全程保持通电与锁定力矩)
    dwt_delay_init(); 
    tph_gpio_init();
    bk_motor_gpio_config_init();

    tph_power_on(); 
    rtos_delay_milliseconds(100); 

    // 唤醒字车电机 U4
    GPIO_UP(TPH_MOT_MS1); GPIO_UP(TPH_MOT_MS2); GPIO_UP(TPH_MOT_MS3);
    GPIO_UP(TPH_MOT_SLEEP);
    bk_delay_us(100);
    GPIO_UP(TPH_MOT_RES);
    GPIO_DOWN(TPH_MOT_EN); 
    rtos_delay_milliseconds(50); 

    // 唤醒进纸电机 U5
    GPIO_UP(PAPER_MOT_MS1); GPIO_UP(PAPER_MOT_MS2); GPIO_UP(PAPER_MOT_MS3);
    GPIO_UP(PAPER_MOT_SLEEP);
    bk_delay_us(100);
    GPIO_UP(PAPER_MOT_RES);
    GPIO_DOWN(PAPER_MOT_EN); 
    rtos_delay_milliseconds(10);

    uint32_t idx = 0;
    uint8_t current_channel = 0;
    uint32_t pass_count = 0;

    memset(s_decoded_line, 0, sizeof(s_decoded_line));

    while (idx < len - 5) {
        if (buf[idx] == 0x1B && buf[idx+1] == 0x56) {
            current_channel = buf[idx+2];
            idx += 3;
            continue;
        }

        if (buf[idx] == 0x1B && buf[idx+1] == 0x25 && buf[idx+2] == 0x32) {
            idx += 5;

            uint32_t data_start = idx;
            while (idx < len && buf[idx] != 0x1B) {
                idx++;
            }
            uint32_t block_size = idx - data_start;

            memset(s_unpacked_swath, 0, sizeof(s_unpacked_swath));
            int unpacked_len = decode_packbits_full(&buf[data_start], block_size, s_unpacked_swath, sizeof(s_unpacked_swath));

            uint32_t bytes_per_col = unpacked_len / 1180;
            if (bytes_per_col == 0) bytes_per_col = 1;

            for (uint32_t col = 0; col < 1180; col++) {
                uint32_t src_offset = col * bytes_per_col;
                uint8_t dot_val = (src_offset < unpacked_len) ? s_unpacked_swath[src_offset] : 0x00;

                if (current_channel == 1) { 
                    for (int b = 0; b < 25; b++) s_decoded_line[col].c_data[b] = dot_val;
                } else if (current_channel == 2) { 
                    if (col >= 12) {
                        for (int b = 0; b < 25; b++) s_decoded_line[col].m_data[b] = dot_val;
                    }
                } else if (current_channel == 3) { 
                    if (col >= 24) {
                        for (int b = 0; b < 25; b++) s_decoded_line[col].y_data[b] = dot_val;
                    }
                }
            }

            // 当 C、M、Y 数据合成完毕，开始单行扫掠
            if (current_channel == 3) {
                pass_count++;
                os_printf(">>> Firing Pass %lu (1180 Cols) -> Advancing Paper %d Steps <<<\r\n", 
                          pass_count, LINE_FEED_STEPS);

                // 1. 字车扫掠打火
                sweep_and_fire_line(s_decoded_line);

                // 2. ★ 精准送纸 2920 微步 (8.46mm)，彻底消除重叠 ★
                feed_paper_calibrated(LINE_FEED_STEPS);

                // 3. 字车平稳归位
                carriage_return_home_fast();

                // 清空准备下一行
                memset(s_decoded_line, 0, sizeof(s_decoded_line));
            }
            continue;
        }

        idx++;
    }

    // 所有行打印完成，统一出纸 5000 步
    os_printf(">>> All Passes Done! Ejecting Paper %d Steps...\r\n", PAPER_EJECT_STEPS);
    feed_paper_calibrated(PAPER_EJECT_STEPS);

    // 硬件安全下电
    GPIO_UP(TPH_MOT_EN);   GPIO_DOWN(TPH_MOT_SLEEP);
    GPIO_UP(PAPER_MOT_EN); GPIO_DOWN(PAPER_MOT_SLEEP);
    tph_power_off();

    os_printf("========================================================\r\n");
    os_printf(">>> Job Completed! Passes: %lu <<<\r\n", pass_count);
    os_printf("========================================================\r\n");
}

/* 打印工作线程 */
static void printer_worker_thread(void *arg)
{
    while (1) {
        rtos_get_semaphore(&s_print_sem, BEKEN_WAIT_FOREVER);

        if (s_print_buffer != NULL && s_buffer_len > 0) {
            parse_and_execute_stream_perfect(s_print_buffer, s_buffer_len);
        }

        hw_printer_clear_buffer();
    }
}

void hw_printer_start_job_async(void)
{
    if (s_print_sem) {
        rtos_set_semaphore(&s_print_sem);
    }
}

void hw_printer_create_task(void)
{
    if (s_print_sem == NULL) {
        rtos_init_semaphore(&s_print_sem, 1);
        rtos_create_thread(&s_print_task_handle, 4, "prt_worker", 
                           printer_worker_thread, 4096, NULL);
    }
}

