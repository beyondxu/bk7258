/**
 * @file printer_protocol.h
 * @brief ESC/POS 打印协议解析器头文件
 */

#ifndef __PRINTER_PROTOCOL_H__
#define __PRINTER_PROTOCOL_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 状态码 (与 CocoPrint App 端匹配)
 *============================================================================*/
typedef enum {
    PRINTER_STATUS_OK           = 0x00,
    PRINTER_STATUS_FAIL         = 0x01,
    PRINTER_STATUS_NOT_CONN     = 0x02,
    PRINTER_STATUS_CONNECTED    = 0x03,
    PRINTER_STATUS_CONNECTING   = 0x04,
    PRINTER_STATUS_DISCONNECTED = 0x05,
    PRINTER_STATUS_NOT_SUPPORT  = 0x06,
    PRINTER_STATUS_LOW_BATTERY  = 0x07,
    PRINTER_STATUS_NO_PAPER     = 0x08,
    PRINTER_STATUS_OVERHEAT     = 0x09,
    PRINTER_STATUS_BUSY         = 0x0A,
} printer_status_t;

/*============================================================================
 * 打印机运行时状态
 *============================================================================*/
typedef struct {
    uint8_t  align;        /* 对齐: 0=左, 1=中, 2=右 */
    bool     bold;         /* 加粗 */
    uint8_t  underline;    /* 下划线: 0=关, 1=开 */
    uint8_t  font_size;    /* 字体大小 (GS ! n) */
    bool     paper_out;    /* 缺纸 */
    bool     overheat;     /* 过热 */
    bool     busy;         /* 忙碌 */
} printer_state_t;

/*============================================================================
 * 公开接口
 *============================================================================*/

/**
 * @brief 初始化协议解析器
 */
void printer_protocol_init(void);

/**
 * @brief 重置协议解析器 (断连时调用)
 */
void printer_protocol_reset(void);

/**
 * @brief 处理从 BLE 收到的数据 (核心入口)
 * @param data  数据指针
 * @param len   数据长度
 */
void printer_process_data(const uint8_t *data, uint32_t len);

/**
 * @brief 获取当前打印机状态
 */
printer_state_t *printer_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* __PRINTER_PROTOCOL_H__ */
