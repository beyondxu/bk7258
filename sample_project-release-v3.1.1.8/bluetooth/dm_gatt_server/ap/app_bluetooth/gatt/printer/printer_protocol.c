#include <string.h>
#include <stdio.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include "printer_protocol.h"
#include "printer_diagnostics.h"
#include "printer_audit_utils.h"
#include "ble_printer_service.h"

#define TAG "PRN_PROTO"
#define FRAME_HEAD  0x1D
#define FRAME_TAIL  0x1E

extern void hw_printer_create_task(void);
extern bool hw_printer_append_data(const uint8_t *data, uint32_t len);
extern void hw_printer_clear_buffer(void);
extern bool hw_printer_prepare_buffer(uint32_t total_len);
extern void hw_printer_start_job_async(void);

static dn20_printer_status_t s_status = {
    .work           = 0x00, // 0x00: 空闲
    .fault          = 0x00, // 0x00: 无故障
    .paper          = 0x00, // 0x00: 有纸
    .cover          = 0x00, // 0x00: 关盖
    .battery        = 98,   // 98% 电量
    .buffer         = 0x01, // 0x01: 缓存充足
    .auto_power_off = 20,   // 20分钟
    .cartridge      = 0x00, // 0x00: 墨盒正常
    .charging       = 0x00, // 0x00: 未充电
    .reserved       = 0x00  // 保留
};

typedef enum {
    JOB_STATE_IDLE = 0,
    JOB_STATE_RECEIVING_BIN,
    JOB_STATE_PRINTING,
} job_state_t;

static struct {
    job_state_t state;
    uint16_t task_id;
    uint32_t total_bin_len;
    uint32_t received_bin_len;
    uint32_t packet_count;
    uint32_t expected_crc;
    uint32_t running_crc;
} s_job = {0};

/**
 * @brief ★ 严格对照协议第 1 章：标准应答帧打包发送函数 ★
 * 格式：[1D] [00] [DataLen(2B)] [Cat] [Sub] [Type] [ParamLen(2B)] [Params(NB)] [CRC(4B)] [1E]
 */
static void send_dn20_response(uint8_t category, uint8_t sub_class, uint8_t type, const uint8_t *param, uint16_t param_len)
{
    uint8_t rsp[128];
    uint16_t idx = 0;

    // 数据长度 = 类别(1) + 子类(1) + 类型(1) + 参数长度(2) + 参数内容(N) = 5 + param_len
    uint16_t data_len = 5 + param_len;

    // 1. 帧首部
    rsp[idx++] = FRAME_HEAD; // 0x1D
    rsp[idx++] = 0x00;       // 预留
    rsp[idx++] = (uint8_t)(data_len & 0xFF);        // 数据长度低字节
    rsp[idx++] = (uint8_t)((data_len >> 8) & 0xFF); // 数据长度高字节

    // 2. 帧数据
    rsp[idx++] = category;   // 类别
    rsp[idx++] = sub_class;  // 子类
    rsp[idx++] = type;       // 类型: 0x01=请求/主动上报, 0x02=应答
    rsp[idx++] = (uint8_t)(param_len & 0xFF);        // 参数长度低字节
    rsp[idx++] = (uint8_t)((param_len >> 8) & 0xFF); // 参数长度高字节

    // 参数内容
    if (param_len > 0 && param != NULL) {
        memcpy(&rsp[idx], param, param_len);
        idx += param_len;
    }

    // 3. 帧尾部：4 字节 CRC 校验占位 (协议填 0x00) + 命令尾 0x1E
    rsp[idx++] = 0x00;
    rsp[idx++] = 0x00;
    rsp[idx++] = 0x00;
    rsp[idx++] = 0x00;
    rsp[idx++] = FRAME_TAIL; // 0x1E

    // 通过 BLE 发送
    printer_ntf_data(rsp, idx);
}

static void printer_process_data_legacy(const uint8_t *data, uint32_t len);
#include "printer_audit_rx.h"

// 主动上报状态 (协议第 5 页：Cat=0x06, Sub=0x02, Type=0x01)
void printer_report_status(void)
{
    send_dn20_response(0x06, 0x02, 0x01, (uint8_t *)&s_status, sizeof(s_status));
}

void printer_protocol_init(void)
{
    audit_frame_used = 0;
    memset(&s_job, 0, sizeof(s_job));
    s_job.state = JOB_STATE_IDLE;
    hw_printer_create_task();
}

void printer_protocol_reset(void)
{
    audit_frame_used = 0;
    if (PRINTER_DATA_AUDIT && s_job.state == JOB_STATE_PRINTING) return;
    memset(&s_job, 0, sizeof(s_job));
    s_job.state = JOB_STATE_IDLE;
    s_status.work = 0x00;
    hw_printer_clear_buffer();
}
// 全局质量变量：默认 para=2 (8刷打印)
uint8_t g_print_quality_pass = 8; 

/**
 * @brief 核心协议解析器
 */
static void printer_process_data_legacy(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0) return;

    // =========================================================================
    // 1. 标准控制帧识别：必须以 0x1D 开头，以 0x1E 结尾，长度 >= 10
    // =========================================================================
    if (data[0] == FRAME_HEAD && data[len - 1] == FRAME_TAIL && len >= 10) {
        uint8_t category   = data[4];
        uint8_t sub_class  = data[5];
        uint8_t cmd_type   = data[6];
        uint16_t param_len = data[7] | (data[8] << 8);
        const uint8_t *params = &data[9];

        // 只处理请求命令 (Type == 0x01)
        if (cmd_type != 0x01) return;

        os_printf("[PROTO] Received Command: Cat=0x%02X, Sub=0x%02X, ParamLen=%d\r\n", 
                  category, sub_class, param_len);

        // -------------------------------------------------------------
        // 第 2 章 1: 查询 SN (Cat 0x01, Sub 0x02) -> 应答带字符串截止符 0x00
        // -------------------------------------------------------------
        if (category == 0x01 && sub_class == 0x02) {
            const char *sn = "DN20-M0333N-001";
            send_dn20_response(0x01, 0x02, 0x02, (const uint8_t *)sn, strlen(sn) + 1);
            return;
        }

        // -------------------------------------------------------------
        // 第 2 章 2: 查询版本号 (Cat 0x01, Sub 0x07)
        // -------------------------------------------------------------
        if (category == 0x01 && sub_class == 0x07) {
            uint8_t ver_buf[64];
            uint16_t v_idx = 0;
            // Tag 0x01: Firmware version
            ver_buf[v_idx++] = 0x01; ver_buf[v_idx++] = 0x05; ver_buf[v_idx++] = 0x00;
            memcpy(&ver_buf[v_idx], "V1.2\0", 5); v_idx += 5;
            // Tag 0x02: Hardware version
            ver_buf[v_idx++] = 0x02; ver_buf[v_idx++] = 0x05; ver_buf[v_idx++] = 0x00;
            memcpy(&ver_buf[v_idx], "V1.0\0", 5); v_idx += 5;
            // Tag 0x03: WiFi/BLE version
            ver_buf[v_idx++] = 0x03; ver_buf[v_idx++] = 0x05; ver_buf[v_idx++] = 0x00;
            memcpy(&ver_buf[v_idx], "V1.0\0", 5); v_idx += 5;

            send_dn20_response(0x01, 0x07, 0x02, ver_buf, v_idx);
            return;
        }

        // -------------------------------------------------------------
        // ★★★ 第 2 章 3: APP 查询状态 (Cat 0x06, Sub 0x01) ★★★
        // 协议第 4 页明确规定：应答必须是 Cat=0x06, Sub=0x01, Type=0x02, 长度=10 字节！
        // -------------------------------------------------------------
        if (category == 0x06 && sub_class == 0x01) {
            os_printf("[PROTO] Responding APP Status (10 Bytes OK)...\r\n");
            send_dn20_response(0x06, 0x01, 0x02, (uint8_t *)&s_status, sizeof(s_status));
            return;
        }

        // -------------------------------------------------------------
        // 第 2 章 5: 【打印任务数据开始发送】(Cat 0x06, Sub 0x03)
        // -------------------------------------------------------------
        if (category == 0x06 && sub_class == 0x03) {
            uint16_t offset = 0;
            while (offset + 3 <= param_len) {
                uint8_t tag = params[offset];
                uint16_t tlen = params[offset + 1] | (params[offset + 2] << 8);
                offset += 3;
                if (tag == 0x01 && tlen == 2) {
                    s_job.task_id = params[offset] | (params[offset+1] << 8);
                } else if (tag == 0x02 && tlen == 4) {
                    s_job.total_bin_len = params[offset] | (params[offset+1] << 8) | 
                                         (params[offset+2] << 16) | (params[offset+3] << 24);
                }
                offset += tlen;
            }

            os_printf("[JOB START] Task ID=0x%04X, Total Bin Len=%lu Bytes\r\n", 
                      s_job.task_id, (unsigned long)s_job.total_bin_len);

            hw_printer_clear_buffer();
            bool mem_ok = hw_printer_prepare_buffer(s_job.total_bin_len);

            s_job.received_bin_len = 0;
            s_job.packet_count = 0;
            s_job.state = JOB_STATE_RECEIVING_BIN;
            s_status.work = 0x01; // 标记忙碌

            // 协议第 6 页：para=0x00 表示可以开始接收
            uint8_t ready_para = mem_ok ? 0x00 : 0x01;
            send_dn20_response(0x06, 0x03, 0x02, &ready_para, 1);
            return;
        }

        // -------------------------------------------------------------
        // 第 2 章 6: 【打印任务数据发送完成】(Cat 0x06, Sub 0x04)
        // -------------------------------------------------------------
        if (category == 0x06 && sub_class == 0x04) {
            os_printf("\r\n>>> [JOB FINISH] App Send Done! Triggering Motor Print! <<<\r\n");

            s_job.state = JOB_STATE_PRINTING;
            hw_printer_start_job_async();

            // 协议第 7 页：应答 Page_ID 与 Result (0x00=成功 / 0x02=正在打印)
            uint8_t rsp[16];
            uint16_t rlen = 0;
            rsp[rlen++] = 0x01; rsp[rlen++] = 0x02; rsp[rlen++] = 0x00;
            rsp[rlen++] = (uint8_t)(s_job.task_id & 0xFF);
            rsp[rlen++] = (uint8_t)((s_job.task_id >> 8) & 0xFF);
            rsp[rlen++] = 0x02; rsp[rlen++] = 0x01; rsp[rlen++] = 0x00;
            rsp[rlen++] = 0x00; // 0x00: 成功开始打印

            send_dn20_response(0x06, 0x04, 0x02, rsp, rlen);

            s_status.work = 0x00;
            s_job.state = JOB_STATE_IDLE;
            return;
        }

        // -------------------------------------------------------------
        // 第 2 章 8: 设置竖线调整参数 (Cat 0x06, Sub 0x06)
        // -------------------------------------------------------------
        if (category == 0x06 && sub_class == 0x06) {
            send_dn20_response(0x06, 0x06, 0x02, NULL, 0);
            return;
        }

        // -------------------------------------------------------------
        // 第 2 章 10: 设置打印质量/多刷参数 (Cat 0x06, Sub 0x07)
        // -------------------------------------------------------------
        if (category == 0x06 && sub_class == 0x07) {
			uint8_t quality_mode = params[0]; // 1=低(4刷), 2=中(8刷), 3=高(16刷)
    
            if (quality_mode == 1)      g_print_quality_pass = 4;
               else if (quality_mode == 2) g_print_quality_pass = 8;
               else if (quality_mode == 3) g_print_quality_pass = 16;
               else                        g_print_quality_pass = 8;

    os_printf("[QUALITY SET] App Set Quality Mode: %d -> Firing %d Passes per Swath!\r\n", 
              quality_mode, g_print_quality_pass);
            send_dn20_response(0x06, 0x07, 0x02, NULL, 0);
            return;
        }

        // -------------------------------------------------------------
        // 第 2 章 11: 设置自动关机时间 (Cat 0x06, Sub 0x09)
        // -------------------------------------------------------------
        if (category == 0x06 && sub_class == 0x09) {
            if (param_len >= 2) {
                s_status.auto_power_off = params[0];
            }
            send_dn20_response(0x06, 0x09, 0x02, NULL, 0);
            return;
        }

        // -------------------------------------------------------------
        // 第 2 章 12: 查询蓝牙 MAC 地址 (Cat 0x01, Sub 0x08)
        // -------------------------------------------------------------
        if (category == 0x01 && sub_class == 0x08) {
            uint8_t mac[6] = {0xC8, 0x47, 0x8C, 0xD8, 0x3D, 0x8A};
            send_dn20_response(0x01, 0x08, 0x02, mac, 6);
            return;
        }

        // 兜底应答未知指令 (协议通用应答，空参数)
        send_dn20_response(category, sub_class, 0x02, NULL, 0);
        return;
    }

    // =========================================================================
    // 2. 纯图片数据流追加：只有在进入接收阶段，且不是控制帧时，才追加进内存！
    // =========================================================================
    if (s_job.state == JOB_STATE_RECEIVING_BIN) {
        s_job.received_bin_len += len;
        s_job.packet_count++;
        hw_printer_append_data(data, (uint16_t)len);

        if (s_job.packet_count % 50 == 0 || s_job.received_bin_len >= s_job.total_bin_len) {
            uint8_t progress = (s_job.total_bin_len > 0) ? (s_job.received_bin_len * 100 / s_job.total_bin_len) : 0;
            os_printf("[Photo Stream] Recv: %lu / %lu Bytes (%d%%)\r\n", 
                      (unsigned long)s_job.received_bin_len, (unsigned long)s_job.total_bin_len, progress);
        }
    }
}


void printer_process_data(const uint8_t *data, uint32_t len)
{
    if (PRINTER_DATA_AUDIT) printer_audit_feed(data, len);
    else printer_process_data_legacy(data, len);
}

void printer_protocol_audit_complete(uint32_t saved_len, uint32_t saved_crc)
{
    bool ok = saved_len == s_job.total_bin_len && saved_crc == s_job.expected_crc;
    os_printf("[AUDIT_RESULT] task=%u len=%lu expected_len=%lu crc32=%08lX expected_crc32=%08lX %s (NO PHYSICAL PRINT)\r\n",
              s_job.task_id, (unsigned long)saved_len, (unsigned long)s_job.total_bin_len,
              (unsigned long)saved_crc, (unsigned long)s_job.expected_crc,
              ok ? "PASS" : "FAIL");
    audit_send_finish(s_job.task_id, ok ? 0 : 1);
    s_status.work = 0;
    s_status.fault = ok ? 0 : 8;
    s_job.state = JOB_STATE_IDLE;
}

/* Bit3 is a DATA/command fault in the DN20 status protocol, not a generic
 * hardware failure. Preserve the failed finish result without inventing
 * paper/motor faults for link failures that have no dedicated status bit.
 */
static uint8_t printer_execution_fault(int result, bool data_ok)
{
    return !data_ok || result == -1 || result == -4 || result == -11 ? 8U : 0U;
}

void printer_protocol_payload_complete(uint32_t saved_len, uint32_t saved_crc, int result)
{
    bool integrity = saved_len == s_job.total_bin_len && saved_crc == s_job.expected_crc;
    bool ok = integrity && result == 0;
    os_printf("[APP_PAYLOAD_RESULT] task=%u code=%d input_integrity=%s decode=%s NO_PHYSICAL_PRINT\r\n",
              s_job.task_id, result, integrity ? "PASS" : "FAIL", result == 0 ? "PASS" : "FAIL");
    s_status.work = 0;
    s_status.fault = printer_execution_fault(result, integrity && result != -3);
    s_job.state = JOB_STATE_IDLE;
    audit_send_finish(s_job.task_id, ok ? 0 : 1);
}

void printer_protocol_bounded_complete(uint32_t saved_len, uint32_t saved_crc, int test_result)
{
    bool ok = test_result == 0 && saved_len == s_job.total_bin_len && saved_crc == s_job.expected_crc;
    os_printf("[LINE_RESULT] task=%u code=%d motion_transport=%s ink_unverified\r\n",
              s_job.task_id, test_result, ok ? "PASS" : "FAIL");
    audit_send_finish(s_job.task_id, ok ? 0 : 1);
    s_status.work = 0;
    s_status.fault = printer_execution_fault(test_result, saved_len == s_job.total_bin_len && saved_crc == s_job.expected_crc);
    s_job.state = JOB_STATE_IDLE;
}

void printer_protocol_page_complete(uint32_t saved_len, uint32_t saved_crc, int result)
{
    bool ok = result == 0 && saved_len == s_job.total_bin_len && saved_crc == s_job.expected_crc;
    os_printf("[PAGE_RESULT] task=%u code=%d transport_motion=%s ink_and_feed_unverified\r\n",
              s_job.task_id, result, ok ? "PASS" : "FAIL");
    audit_send_finish(s_job.task_id, ok ? 0 : 1);
    s_status.work = 0;
    s_status.fault = printer_execution_fault(result, saved_len == s_job.total_bin_len && saved_crc == s_job.expected_crc);
    if (!ok) os_printf("[PAGE_FAULT] task=%u code=%d input_integrity=%s status_fault=%u finish=FAIL\r\n",
                       s_job.task_id, result,
                       saved_len == s_job.total_bin_len && saved_crc == s_job.expected_crc ? "PASS" : "FAIL",
                       (unsigned)s_status.fault);
    s_job.state = JOB_STATE_IDLE;
}

bool printer_protocol_job_buffer_valid(uint32_t saved_len, uint32_t saved_crc)
{
    return s_job.state == JOB_STATE_PRINTING && saved_len == s_job.total_bin_len && saved_crc == s_job.expected_crc;
}
