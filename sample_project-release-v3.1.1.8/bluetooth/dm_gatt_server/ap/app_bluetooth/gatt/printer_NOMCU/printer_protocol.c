#include <string.h>
#include <stdio.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include "printer_protocol.h"
#include "ble_printer_service.h"

#define TAG "PRN_PROTO"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define FRAME_HEAD  0x1D
#define FRAME_TAIL  0x1E

/* 外部硬件驱动接口声明 */
extern void hw_printer_init(void);
extern void hw_printer_create_task(void);
extern void hw_printer_append_data(const uint8_t *data, uint16_t len);
extern void hw_printer_clear_buffer(void);
/* 在文件开头的 extern 区域补上这一行声明： */
extern bool hw_printer_prepare_buffer(uint32_t total_len);

extern void hw_printer_start_job_async(void);
static dn20_printer_status_t s_status = {
    .work = 0x00,           // 0x00: 空闲(idle)
    .fault = 0x00,          // 0x00: 无故障
    .paper = 0x00,          // 0x00: 有纸
    .cover = 0x00,          // 0x00: 关盖
    .battery = 100,         // 100% 电量
    .buffer = 0x01,         // 充足
    .auto_power_off = 20,   // 20分钟
    .cartridge = 0x00,      // 正常
    .charging = 0x00,
    .reserved = 0x00
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
    uint32_t data_crc;
    uint8_t  quality_pass;
    uint32_t packet_count;
} s_job = {0};

/* 回复 APP 协议帧封装 */
static void send_dn20_response(uint8_t category, uint8_t sub_class, uint8_t type, const uint8_t *param, uint16_t param_len)
{
    uint8_t rsp[128];
    uint16_t data_len = 5 + param_len;
    uint16_t idx = 0;

    rsp[idx++] = FRAME_HEAD;
    rsp[idx++] = 0x00;
    rsp[idx++] = (uint8_t)(data_len & 0xFF);
    rsp[idx++] = (uint8_t)((data_len >> 8) & 0xFF);
    rsp[idx++] = category;
    rsp[idx++] = sub_class;
    rsp[idx++] = type;
    rsp[idx++] = (uint8_t)(param_len & 0xFF);
    rsp[idx++] = (uint8_t)((param_len >> 8) & 0xFF);

    if (param_len > 0 && param != NULL) {
        memcpy(&rsp[idx], param, param_len);
        idx += param_len;
    }

    // 4 字节 CRC 占位
    rsp[idx++] = 0x00;
    rsp[idx++] = 0x00;
    rsp[idx++] = 0x00;
    rsp[idx++] = 0x00;
    rsp[idx++] = FRAME_TAIL;

    printer_ntf_data(rsp, idx);
}

void printer_protocol_init(void)
{
    memset(&s_job, 0, sizeof(s_job));
    s_job.state = JOB_STATE_IDLE;
    
    // 初始化硬件与后台打印任务
  //  hw_printer_init();
    hw_printer_create_task();

    LOGI("DN20 Protocol parser and Printer Hardware Ready\r\n");
}

void printer_protocol_reset(void)
{
    memset(&s_job, 0, sizeof(s_job));
    s_job.state = JOB_STATE_IDLE;
    s_status.work = 0x00;
    hw_printer_clear_buffer();
    LOGI("DN20 Protocol parser reset to IDLE\r\n");
}

/* 核心解析入口 */
void printer_process_data(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0) return;

    // 1. 如果处于 BIN 数据流接收阶段，且数据不是以 0x1D 开头的协议控制帧，则属于照片点阵数据
    if (s_job.state == JOB_STATE_RECEIVING_BIN) {
        if (data[0] != FRAME_HEAD) {
            s_job.received_bin_len += len;
            s_job.packet_count++;

            // 将点阵数据存入 PSRAM 缓存
            hw_printer_append_data(data, (uint16_t)len);

            // 每 50 包打印一次接收进度
            if (s_job.packet_count % 50 == 0 || s_job.received_bin_len >= s_job.total_bin_len) {
                uint8_t progress = (s_job.total_bin_len > 0) ? (s_job.received_bin_len * 100 / s_job.total_bin_len) : 0;
                LOGI("Receiving Photo: Pkt=%lu, %lu / %lu Bytes (%d%%)\r\n", 
                     (unsigned long)s_job.packet_count, 
                     (unsigned long)s_job.received_bin_len, 
                     (unsigned long)s_job.total_bin_len, 
                     progress);
            }
            return;
        }
    }

    // 2. 检查是否为标准控制帧 (至少 14 字节，且满足 0x1D ... 0x1E)
    if (len >= 14 && data[0] == FRAME_HEAD && data[len - 1] == FRAME_TAIL) {
        uint8_t category   = data[4];
        uint8_t sub_class  = data[5];
        uint8_t cmd_type   = data[6];
        uint16_t param_len = data[7] | (data[8] << 8);
        const uint8_t *params = &data[9];

        if (cmd_type != 0x01) return; // 只响应 APP 的请求 (0x01)

        /* ================= 类别 0x01: 系统信息 ================= */
        if (category == 0x01) {
            if (sub_class == 0x02) { /* 查询 SN */
                const char *sn = "DN20-BK7258-001";
                send_dn20_response(0x01, 0x02, 0x02, (const uint8_t *)sn, strlen(sn) + 1);
            }
            else if (sub_class == 0x07) { /* 查询版本号 */
                uint8_t ver[64];
                uint16_t vlen = 0;
                const char *str1 = "1.0.0";
                const char *str2 = "1.0.0";
                const char *str3 = "1.0.0";

                ver[vlen++] = 0x01; ver[vlen++] = strlen(str1)+1; ver[vlen++] = 0;
                memcpy(&ver[vlen], str1, strlen(str1)+1); vlen += strlen(str1)+1;

                ver[vlen++] = 0x02; ver[vlen++] = strlen(str2)+1; ver[vlen++] = 0;
                memcpy(&ver[vlen], str2, strlen(str2)+1); vlen += strlen(str2)+1;

                ver[vlen++] = 0x03; ver[vlen++] = strlen(str3)+1; ver[vlen++] = 0;
                memcpy(&ver[vlen], str3, strlen(str3)+1); vlen += strlen(str3)+1;

                send_dn20_response(0x01, 0x07, 0x02, ver, vlen);
            }
            else if (sub_class == 0x08) { /* 查询 MAC 地址 */
                uint8_t mac[6] = {0xC8, 0x47, 0x8C, 0x00, 0x11, 0x22};
                send_dn20_response(0x01, 0x08, 0x02, mac, 6);
            }
        }
        /* ================= 类别 0x06: 打印控制 ================= */
        else if (category == 0x06) {
            if (sub_class == 0x01) { /* 查询状态 */
                LOGI("-> Handled: Query Status -> Reply Ready!\r\n");
                send_dn20_response(0x06, 0x01, 0x02, (uint8_t *)&s_status, sizeof(s_status));
            }
            else if (sub_class == 0x07) { /* 设置打印质量 (刷数) */
                if (param_len >= 1) {
                    s_job.quality_pass = params[0];
                    LOGI("-> Handled: Set Quality Pass=%d\r\n", s_job.quality_pass);
                }
                send_dn20_response(0x06, 0x07, 0x02, NULL, 0);
            }
			  else if (sub_class == 0x03) { /* 打印任务开始 */
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
								  } else if (tag == 0x03 && tlen == 4) {
									  s_job.data_crc = params[offset] | (params[offset+1] << 8) | 
													   (params[offset+2] << 16) | (params[offset+3] << 24);
								  }
								  offset += tlen;
							  }
			  
							  LOGI("Start Print Job: ID=0x%04X, TotalLen=%lu Bytes. Preparing Buffer...\r\n", 
								   s_job.task_id, (unsigned long)s_job.total_bin_len);
			  
							  // 申请 PSRAM 内存
							  bool mem_ok = hw_printer_prepare_buffer(s_job.total_bin_len);
			  
							  s_job.received_bin_len = 0;
							  s_job.packet_count = 0;
							  s_job.state = JOB_STATE_RECEIVING_BIN;
							  s_status.work = 0x01; // 标记忙碌
			  
							  // 回复 0x00 允许接收
							  uint8_t ready = mem_ok ? 0x00 : 0x01;
							  send_dn20_response(0x06, 0x03, 0x02, &ready, 1);
						  }


			  else if (sub_class == 0x04) { /* 打印任务数据发送完成命令 */
                LOGI("Finish Command Received! Total Recv=%lu Bytes. Triggering Physical Print...\r\n", 
                     (unsigned long)s_job.received_bin_len);

                s_job.state = JOB_STATE_PRINTING;

                // 唤醒后台打印线程，开始马达走纸和喷墨打火！
                hw_printer_start_job_async();

                // 准备完成回复: Tag1=TaskID, Tag2=Result(0x00成功)
                uint8_t rsp[16];
                uint16_t rlen = 0;

                rsp[rlen++] = 0x01; rsp[rlen++] = 0x02; rsp[rlen++] = 0x00;
                rsp[rlen++] = (uint8_t)(s_job.task_id & 0xFF);
                rsp[rlen++] = (uint8_t)((s_job.task_id >> 8) & 0xFF);

                rsp[rlen++] = 0x02; rsp[rlen++] = 0x01; rsp[rlen++] = 0x00;
                rsp[rlen++] = 0x00; // 0x00: 成功

                send_dn20_response(0x06, 0x04, 0x02, rsp, rlen);

                s_status.work = 0x00; // 恢复空闲
                s_job.state = JOB_STATE_IDLE;
            }
            else if (sub_class == 0x08) { /* 清洗喷头 */
                LOGI("-> Handled: Clean Head\r\n");
                uint8_t res = 0x00;
                send_dn20_response(0x06, 0x08, 0x02, &res, 1);
            }
        }
        return;
    }

    // 3. 兜底点阵流写入
    if (s_job.state == JOB_STATE_RECEIVING_BIN) {
        s_job.received_bin_len += len;
        s_job.packet_count++;
        hw_printer_append_data(data, (uint16_t)len);
    }
}

