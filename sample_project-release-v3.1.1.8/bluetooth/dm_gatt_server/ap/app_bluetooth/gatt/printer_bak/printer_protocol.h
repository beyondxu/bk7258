#ifndef __PRINTER_PROTOCOL_H__
#define __PRINTER_PROTOCOL_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 协议状态结构体 */
typedef struct {
    uint8_t work;           // 0x00=空闲, 0x01=忙碌
    uint8_t fault;          // 故障码
    uint8_t paper;          // 0x00=有纸, 0x01=缺纸
    uint8_t cover;          // 0x00=关盖, 0x01=开盖
    uint8_t battery;        // 电量百分比 0~100
    uint8_t buffer;         // 0x00=空, 0x01=充足, 0x02=不足
    uint8_t auto_power_off; // 自动关机时间(分钟)
    uint8_t cartridge;      // 0x00=正常, 0x01=无墨盒, 0x02=墨盒异常
    uint8_t charging;       // 0x00=未充电, 0x01=充电中
    uint8_t reserved;       // 保留
} __attribute__((packed)) dn20_printer_status_t;

void printer_protocol_init(void);
void printer_protocol_reset(void);
void printer_process_data(const uint8_t *data, uint32_t len);

/* 供其他模块更新打印机物理状态 */
void printer_set_status(const dn20_printer_status_t *status);
void printer_report_status(void);

#ifdef __cplusplus
}
#endif

#endif /* __PRINTER_PROTOCOL_H__ */

