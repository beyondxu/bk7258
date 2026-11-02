/**
 * @file printer_protocol.c
 * @brief ESC/POS 打印协议解析器
 * @note 支持 CocoPrint App (dnInkPrinterSDK) 发送的全部指令:
 *       ESC @ (1B 40)       初始化
 *       ESC a n (1B 61 n)   对齐
 *       LF (0A)             换行
 *       ESC i (1B 69)       切纸
 *       ESC d n (1B 64 n)   走纸
 *       ESC E n (1B 45 n)   加粗
 *       ESC - n (1B 2D n)   下划线
 *       GS ! n (1D 21 n)    字体大小
 *       GS v 0 (1D 76 30)   位图打印
 *       GS ( k (1D 28 6B)   二维码
 *       GS k (1D 6B)        条形码
 *       ESC B n t (1B 42)   蜂鸣
 *       ESC p (1B 70)       钱箱
 *       GS r n (1D 72 n)    状态查询
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <components/log.h>

#include "printer_protocol.h"
#include "ble_printer_service.h"

#define TAG "PRN_PROTO"

/*============================================================================
 * 硬件驱动层接口 (由 hw_printer_stub.c 实现)
 *============================================================================*/
extern void hw_printer_init(void);
extern void hw_printer_feed(uint8_t lines);
extern void hw_printer_cut(void);
extern void hw_printer_beep(uint8_t times, uint8_t duration);
extern void hw_printer_cashbox(uint8_t pin, uint8_t t1, uint8_t t2);
extern void hw_printer_print_bitmap(const uint8_t *data,
                                     uint16_t width_bytes,
                                     uint16_t height,
                                     uint8_t mode);
extern void hw_printer_print_text(const char *text, uint16_t len,
                                   uint8_t align, bool bold,
                                   uint8_t underline, uint8_t font_size);
extern void hw_printer_print_qrcode(const uint8_t *data, uint16_t len,
                                     uint8_t size, uint8_t align);
extern void hw_printer_print_barcode(const uint8_t *data, uint8_t type,
                                      uint8_t height, uint8_t width,
                                      uint8_t align);

/*============================================================================
 * 全局状态
 *============================================================================*/
static printer_state_t g_state = {0};

/*============================================================================
 * 解析器状态机
 *============================================================================*/
typedef enum {
    ST_IDLE,              /* 空闲 */
    ST_ESC,               /* 收到 0x1B */
    ST_ESC_PARAM,         /* ESC 指令收集参数 */
    ST_GS,                /* 收到 0x1D */
    ST_GS_PARAM,          /* GS 指令收集参数 */
    ST_GS_V,              /* GS v 模式 */
    ST_GS_V_HEADER,       /* 位图头部参数 */
    ST_GS_V_DATA,         /* 位图数据 */
    ST_GS_PAREN,          /* GS ( 功能命令 */
    ST_GS_PAREN_PARAM,    /* GS ( k 参数 */
    ST_GS_QR_DATA,        /* 二维码数据 */
    ST_GS_BARCODE,        /* GS k 条形码 */
    ST_GS_BARCODE_DATA,   /* 条形码数据 */
    ST_TEXT_BUF,          /* 文本缓冲中 */
} parse_state_t;

/* 位图缓冲区 (最大 384x384 单色 = 18432 bytes) */
#define MAX_BMP_SIZE  (384 * 384 / 8)
static uint8_t g_bmp_buf[MAX_BMP_SIZE];

/* 二维码缓冲区 */
#define MAX_QR_SIZE  512
static uint8_t g_qr_buf[MAX_QR_SIZE];

/* 条形码缓冲区 */
#define MAX_BARCODE_SIZE  64
static uint8_t g_barcode_buf[MAX_BARCODE_SIZE];

/* 文本缓冲区 */
#define MAX_TEXT_SIZE  256
static uint8_t g_text_buf[MAX_TEXT_SIZE];

/* 解析器上下文 */
typedef struct {
    parse_state_t state;
    uint8_t  cmd;              /* 当前指令第二字节 */
    uint8_t  params[8];        /* 参数缓存 */
    uint8_t  param_idx;
    uint8_t  param_need;

    /* 位图 */
    uint8_t  bmp_mode;
    uint16_t bmp_width;        /* 字节数 */
    uint16_t bmp_height;       /* 像素行 */
    uint32_t bmp_total;
    uint32_t bmp_received;

    /* 二维码 */
    uint16_t qr_len;
    uint16_t qr_received;
    uint8_t  qr_size;

    /* 条形码 */
    uint8_t  barcode_type;
    uint8_t  barcode_height;
    uint8_t  barcode_width;
    uint16_t barcode_received;

    /* 文本 */
    uint16_t text_len;
} parser_ctx_t;

static parser_ctx_t g_parser = {0};

/*============================================================================
 * 发送状态通知给 App
 *============================================================================*/
static void _send_status(printer_status_t status)
{
    uint8_t buf[1] = {(uint8_t)status};
    printer_ntf_data(buf, 1);
}

/*============================================================================
 * 刷新文本缓冲区 (遇到控制字符时调用)
 *============================================================================*/
static void _flush_text(void)
{
    if (g_parser.text_len > 0) {
        hw_printer_print_text((const char *)g_text_buf,
                               g_parser.text_len,
                               g_state.align,
                               g_state.bold,
                               g_state.underline,
                               g_state.font_size);
        g_parser.text_len = 0;
    }
}

/*============================================================================
 * 处理 ESC 指令
 *============================================================================*/
static void _handle_esc(uint8_t sub_cmd, uint8_t *params)
{
    switch (sub_cmd) {
    case 0x40:  /* ESC @ - 初始化 */
        _flush_text();
        hw_printer_init();
        g_state.align = 0;
        g_state.bold = false;
        g_state.underline = 0;
        g_state.font_size = 0;
        _send_status(PRINTER_STATUS_OK);
        break;

    case 0x61:  /* ESC a n - 对齐 */
        _flush_text();
        g_state.align = params[0];
        break;

    case 0x64:  /* ESC d n - 走纸 */
        _flush_text();
        hw_printer_feed(params[0]);
        break;

    case 0x69:  /* ESC i - 切纸 */
        _flush_text();
        hw_printer_cut();
        _send_status(PRINTER_STATUS_OK);
        break;

    case 0x45:  /* ESC E n - 加粗 */
        _flush_text();
        g_state.bold = (params[0] != 0);
        break;

    case 0x2D:  /* ESC - n - 下划线 */
        _flush_text();
        g_state.underline = params[0];
        break;

    case 0x42:  /* ESC B n t - 蜂鸣 */
        _flush_text();
        hw_printer_beep(params[0], params[1]);
        break;

    case 0x70:  /* ESC p m t1 t2 - 钱箱 */
        _flush_text();
        hw_printer_cashbox(params[0], params[1], params[2]);
        break;

    default:
        BK_LOGW(TAG, "Unknown ESC cmd: 0x%02X\r\n", sub_cmd);
        break;
    }
}

/*============================================================================
 * 处理 GS 指令
 *============================================================================*/
static void _handle_gs(uint8_t sub_cmd, uint8_t *params)
{
    switch (sub_cmd) {
    case 0x21:  /* GS ! n - 字体大小 */
        _flush_text();
        g_state.font_size = params[0];
        break;

    case 0x72:  /* GS r n - 状态查询 */
    {
        _flush_text();
        printer_status_t st = PRINTER_STATUS_OK;
        if (g_state.paper_out)  st = PRINTER_STATUS_NO_PAPER;
        else if (g_state.overheat) st = PRINTER_STATUS_OVERHEAT;
        else if (g_state.busy) st = PRINTER_STATUS_BUSY;
        _send_status(st);
        break;
    }

    default:
        BK_LOGW(TAG, "Unknown GS cmd: 0x%02X\r\n", sub_cmd);
        break;
    }
}

/*============================================================================
 * 完成位图打印
 *============================================================================*/
static void _finish_bitmap(void)
{
    BK_LOGI(TAG, "Bitmap: %dx%d px, mode=%d, %lu bytes\r\n",
            g_parser.bmp_width * 8, g_parser.bmp_height,
            g_parser.bmp_mode, (unsigned long)g_parser.bmp_total);

    hw_printer_print_bitmap(g_bmp_buf,
                             g_parser.bmp_width,
                             g_parser.bmp_height,
                             g_parser.bmp_mode);
    _send_status(PRINTER_STATUS_OK);
}

/*============================================================================
 * 完成二维码打印
 *============================================================================*/
static void _finish_qrcode(void)
{
    BK_LOGI(TAG, "QRCode: len=%d, size=%d\r\n",
            g_parser.qr_len, g_parser.qr_size);

    hw_printer_print_qrcode(g_qr_buf, g_parser.qr_len,
                             g_parser.qr_size, g_state.align);
    _send_status(PRINTER_STATUS_OK);
}

/*============================================================================
 * 完成条形码打印
 *============================================================================*/
static void _finish_barcode(void)
{
    BK_LOGI(TAG, "Barcode: type=%d, len=%d\r\n",
            g_parser.barcode_type, g_parser.barcode_received);

    hw_printer_print_barcode(g_barcode_buf,
                              g_parser.barcode_type,
                              g_parser.barcode_height,
                              g_parser.barcode_width,
                              g_state.align);
    _send_status(PRINTER_STATUS_OK);
}

/*============================================================================
 * 核心解析函数
 *============================================================================*/
void printer_process_data(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uint8_t b = data[i];

        switch (g_parser.state) {

        /*==== 空闲: 识别指令头 ====*/
        case ST_IDLE:
            if (b == 0x1B) {
                _flush_text();
                g_parser.state = ST_ESC;
            } else if (b == 0x1D) {
                _flush_text();
                g_parser.state = ST_GS;
            } else if (b == 0x0A) {
                _flush_text();
                hw_printer_feed(1);
            } else {
                /* 普通文本字节, 累积到缓冲区 */
                if (g_parser.text_len < MAX_TEXT_SIZE) {
                    g_text_buf[g_parser.text_len++] = b;
                } else {
                    _flush_text();
                    g_text_buf[g_parser.text_len++] = b;
                }
            }
            break;

        /*==== ESC 指令: 识别子命令 ====*/
        case ST_ESC:
            g_parser.cmd = b;
            g_parser.param_idx = 0;
            g_parser.param_need = 0;

            switch (b) {
            case 0x40: /* ESC @ 无参数 */
                _handle_esc(b, NULL);
                g_parser.state = ST_IDLE;
                break;
            case 0x69: /* ESC i 无参数 */
                _handle_esc(b, NULL);
                g_parser.state = ST_IDLE;
                break;
            case 0x61: /* ESC a n */
            case 0x64: /* ESC d n */
            case 0x45: /* ESC E n */
            case 0x2D: /* ESC - n */
                g_parser.param_need = 1;
                g_parser.state = ST_ESC_PARAM;
                break;
            case 0x42: /* ESC B n t */
                g_parser.param_need = 2;
                g_parser.state = ST_ESC_PARAM;
                break;
            case 0x70: /* ESC p m t1 t2 */
                g_parser.param_need = 3;
                g_parser.state = ST_ESC_PARAM;
                break;
            default:
                BK_LOGW(TAG, "Unknown ESC sub: 0x%02X\r\n", b);
                g_parser.state = ST_IDLE;
                break;
            }
            break;

        /*==== ESC 参数收集 ====*/
        case ST_ESC_PARAM:
            g_parser.params[g_parser.param_idx++] = b;
            if (g_parser.param_idx >= g_parser.param_need) {
                _handle_esc(g_parser.cmd, g_parser.params);
                g_parser.state = ST_IDLE;
            }
            break;

        /*==== GS 指令: 识别子命令 ====*/
        case ST_GS:
            g_parser.cmd = b;
            g_parser.param_idx = 0;
            g_parser.param_need = 0;

            switch (b) {
            case 0x76: /* GS v - 位图 */
                g_parser.state = ST_GS_V;
                break;
            case 0x21: /* GS ! n */
                g_parser.param_need = 1;
                g_parser.state = ST_GS_PARAM;
                break;
            case 0x72: /* GS r n */
                g_parser.param_need = 1;
                g_parser.state = ST_GS_PARAM;
                break;
            case 0x28: /* GS ( - 功能命令 */
                g_parser.state = ST_GS_PAREN;
                break;
            case 0x6B: /* GS k - 条形码 */
                g_parser.state = ST_GS_BARCODE;
                g_parser.param_idx = 0;
                g_parser.param_need = 2;
                break;
            default:
                BK_LOGW(TAG, "Unknown GS sub: 0x%02X\r\n", b);
                g_parser.state = ST_IDLE;
                break;
            }
            break;

        /*==== GS 参数收集 ====*/
        case ST_GS_PARAM:
            g_parser.params[g_parser.param_idx++] = b;
            if (g_parser.param_idx >= g_parser.param_need) {
                _handle_gs(g_parser.cmd, g_parser.params);
                g_parser.state = ST_IDLE;
            }
            break;

        /*==== GS v: 位图模式 ====*/
        case ST_GS_V:
            if (b == 0x30) { /* '0' = 位图 */
                g_parser.state = ST_GS_V_HEADER;
                g_parser.param_idx = 0;
                g_parser.param_need = 5; /* mode + xL + xH + yL + yH */
            } else {
                g_parser.state = ST_IDLE;
            }
            break;

        case ST_GS_V_HEADER:
            g_parser.params[g_parser.param_idx++] = b;
            if (g_parser.param_idx >= g_parser.param_need) {
                g_parser.bmp_mode   = g_parser.params[0];
                g_parser.bmp_width  = (g_parser.params[2] << 8) |
                                       g_parser.params[1];
                g_parser.bmp_height = (g_parser.params[4] << 8) |
                                       g_parser.params[3];
                g_parser.bmp_total  = (uint32_t)g_parser.bmp_width *
                                       g_parser.bmp_height;
                g_parser.bmp_received = 0;

                if (g_parser.bmp_total > MAX_BMP_SIZE) {
                    BK_LOGE(TAG, "Bitmap too large: %lu > %d\r\n",
                            (unsigned long)g_parser.bmp_total, MAX_BMP_SIZE);
                    _send_status(PRINTER_STATUS_FAIL);
                    g_parser.state = ST_IDLE;
                } else {
                    BK_LOGI(TAG, "Bitmap header: mode=%d, w=%d, h=%d, "
                            "total=%lu\r\n",
                            g_parser.bmp_mode, g_parser.bmp_width,
                            g_parser.bmp_height,
                            (unsigned long)g_parser.bmp_total);
                    g_parser.state = ST_GS_V_DATA;
                }
            }
            break;

        case ST_GS_V_DATA:
            g_bmp_buf[g_parser.bmp_received++] = b;
            if (g_parser.bmp_received >= g_parser.bmp_total) {
                _finish_bitmap();
                g_parser.state = ST_IDLE;
            }
            break;

        /*==== GS ( : 功能命令 (二维码等) ====*/
        case ST_GS_PAREN:
            /* 下一字节是功能码 */
            if (b == 0x6B) { /* 'k' = QR Code */
                g_parser.state = ST_GS_PAREN_PARAM;
                g_parser.param_idx = 0;
                g_parser.param_need = 4; /* pL pH cn fn */
            } else {
                g_parser.state = ST_IDLE;
            }
            break;

        case ST_GS_PAREN_PARAM:
            g_parser.params[g_parser.param_idx++] = b;
            if (g_parser.param_idx >= g_parser.param_need) {
                /* pL pH cn fn */
                uint16_t pL = g_parser.params[0];
                uint16_t pH = g_parser.params[1];
                /* cn = params[2], fn = params[3] */
                g_parser.qr_len = (pH << 8) | pL;
                g_parser.qr_received = 0;
                g_parser.qr_size = 6; /* 默认模块大小 */

                if (g_parser.qr_len > MAX_QR_SIZE) {
                    BK_LOGE(TAG, "QR too large: %d\r\n", g_parser.qr_len);
                    _send_status(PRINTER_STATUS_FAIL);
                    g_parser.state = ST_IDLE;
                } else {
                    g_parser.state = ST_GS_QR_DATA;
                }
            }
            break;

        case ST_GS_QR_DATA:
            g_qr_buf[g_parser.qr_received++] = b;
            if (g_parser.qr_received >= g_parser.qr_len) {
                _finish_qrcode();
                g_parser.state = ST_IDLE;
            }
            break;

        /*==== GS k: 条形码 ====*/
        case ST_GS_BARCODE:
            g_parser.params[g_parser.param_idx++] = b;
            if (g_parser.param_idx >= g_parser.param_need) {
                g_parser.barcode_type = g_parser.params[0];
                g_parser.barcode_height = 80;
                g_parser.barcode_width = 2;
                g_parser.barcode_received = 0;
                g_parser.state = ST_GS_BARCODE_DATA;
            }
            break;

        case ST_GS_BARCODE_DATA:
            if (b == 0x00 || g_parser.barcode_received >= MAX_BARCODE_SIZE) {
                /* NUL 终止或缓冲区满 */
                _finish_barcode();
                g_parser.state = ST_IDLE;
            } else {
                g_barcode_buf[g_parser.barcode_received++] = b;
                /* 如果有长度参数, 检查是否收完 */
                uint8_t bc_len = g_parser.params[1];
                if (bc_len > 0 && g_parser.barcode_received >= bc_len) {
                    _finish_barcode();
                    g_parser.state = ST_IDLE;
                }
            }
            break;

        default:
            g_parser.state = ST_IDLE;
            break;
        }
    }
}

/*============================================================================
 * 初始化 / 重置
 *============================================================================*/
void printer_protocol_init(void)
{
    memset(&g_parser, 0, sizeof(g_parser));
    g_parser.state = ST_IDLE;
    memset(&g_state, 0, sizeof(g_state));
    BK_LOGI(TAG, "Protocol parser initialized\r\n");
}

void printer_protocol_reset(void)
{
    _flush_text();
    memset(&g_parser, 0, sizeof(g_parser));
    g_parser.state = ST_IDLE;
    memset(&g_state, 0, sizeof(g_state));
    BK_LOGI(TAG, "Protocol parser reset\r\n");
}

/* 补全: 返回当前打印机状态指针 */
printer_state_t *printer_get_state(void)
{
    return &g_state;
}
