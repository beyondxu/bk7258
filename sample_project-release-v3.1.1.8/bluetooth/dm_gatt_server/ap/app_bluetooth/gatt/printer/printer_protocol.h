#ifndef __PRINTER_PROTOCOL_H__
#define __PRINTER_PROTOCOL_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 严格对照协议第 4 页与第 5 页定义的 10 字节状态结构体 (小端模式)
typedef struct {
    uint8_t work;           // p1: 0x00=idle, 0x01=busy
    uint8_t fault;          // p2: 0x00=无故障, Bit0~Bit4对应卡纸/乱码等
    uint8_t paper;          // p3: 0x00=有纸, 0x01=缺纸
    uint8_t cover;          // p4: 0x00=关盖, 0x01=开盖
    uint8_t battery;        // p5: 电池电量百分比 (0~100)
    uint8_t buffer;         // p6: 0x00=空, 0x01=充足, 0x02=不足
    uint8_t auto_power_off; // p7: 自动关机时间 (分钟)
    uint8_t cartridge;      // p8: 0x00=墨盒正常, 0x01=没检测到, 0x02=异常
    uint8_t charging;       // p9: 0x00=未充电, 0x01=充电中
    uint8_t reserved;       // p10: 保留字节 (固定 0x00)
} __attribute__((packed)) dn20_printer_status_t;

void printer_protocol_init(void);
void printer_protocol_reset(void);
void printer_process_data(const uint8_t *data, uint32_t len);

/* 状态更新与主动上报 */
void printer_set_status(const dn20_printer_status_t *status);
void printer_report_status(void);
/* Called after the audit worker has released its buffer. */
void printer_protocol_audit_complete(uint32_t saved_len, uint32_t saved_crc);

#ifdef __cplusplus
}
#endif

#endif /* __PRINTER_PROTOCOL_H__ */

