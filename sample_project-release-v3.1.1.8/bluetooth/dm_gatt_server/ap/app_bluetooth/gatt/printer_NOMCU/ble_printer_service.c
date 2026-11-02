#include <common/sys_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"
#include "dm_gatts.h"
#include "dm_gatt_connection.h"
#include "dm_gap_utils.h"
#include "dm_gatt.h"
#include "ble_printer_service.h"
#include "printer_protocol.h"
#include "bk_cli.h"

#define CHAR_BUFFER_SIZE            512
#define INVALID_ATTR_HANDLE         0

/* 
 * APK 绑定的标准 128-bit 基础 UUID: 0000xxxx-0000-1000-8000-00805F9B34FB
 * 小端字节序排列 (LSB First)
 */
const uint8_t PRINTER_SVC_UUID_128[16] = 
{0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00}; // 0xFF00

const uint8_t PRINTER_NOTIFY_UUID_128[16] = 
{0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00}; // 0xFF01

const uint8_t PRINTER_WRITE_UUID_128[16] = 
{0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x02, 0xFF, 0x00, 0x00}; // 0xFF02

const uint8_t PRINTER_NOTIFY_EXTRA_UUID_128[16] = 
{0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0x00, 0x00}; // 0xFF03

enum 
{
    PRINTER_HDL_IDX_SVC = 0,
    PRINTER_HDL_IDX_NOTIFY,
    PRINTER_HDL_IDX_NOTIFY_DESC,
    PRINTER_HDL_IDX_WRITE,
    PRINTER_HDL_IDX_NOTIFY_EXTRA,
    PRINTER_HDL_IDX_NOTIFY_EXTRA_DESC,
    PRINTER_HDL_IDX_MAX,
};

static uint8_t _char_write_buffer[CHAR_BUFFER_SIZE];
static uint8_t _char_notify_buffer[CHAR_BUFFER_SIZE];
static uint8_t _char_notify_desc_buffer[2];
static uint8_t _char_notify_extra_desc_buffer[2];

static uint8_t s_gatts_if = 0;
static uint16_t gatts_conn_id = 0;
static uint16_t _server_printer_attr_handle_list[PRINTER_HDL_IDX_MAX] = {0};

/* GATT 属性数据库表 */
static const bk_gatts_attr_db_t _printer_attr_db_service[] =
{
    // 0. 主服务: 0xFF00
    {
        BK_GATT_PRIMARY_SERVICE_DECL_128(PRINTER_SVC_UUID_128),
    },

    // 1. 特征值 0xFF01 (Notify: 打印机 -> APP 上报与应答)
    {
        BK_GATT_CHAR_DECL_128(PRINTER_NOTIFY_UUID_128,
                              CHAR_BUFFER_SIZE, _char_notify_buffer,
                              BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                              BK_GATT_PERM_READ,
                              BK_GATT_RSP_BY_APP),
    },
    // 2. 0xFF01 CCCD
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(_char_notify_desc_buffer), _char_notify_desc_buffer,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_RSP_BY_APP),
    },

    // 3. 特征值 0xFF02 (Write: APP -> 打印机 发送控制命令与点阵流)
    {
        BK_GATT_CHAR_DECL_128(PRINTER_WRITE_UUID_128,
                              CHAR_BUFFER_SIZE, _char_write_buffer,
                              BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_WRITE_NR,
                              BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                              BK_GATT_RSP_BY_APP),
    },

    // 4. 特征值 0xFF03 (NotifyExtra: 备用通知)
    {
        BK_GATT_CHAR_DECL_128(PRINTER_NOTIFY_EXTRA_UUID_128,
                              CHAR_BUFFER_SIZE, _char_notify_buffer,
                              BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                              BK_GATT_PERM_READ,
                              BK_GATT_RSP_BY_APP),
    },
    // 5. 0xFF03 CCCD
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(_char_notify_extra_desc_buffer), _char_notify_extra_desc_buffer,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_RSP_BY_APP),
    },
};

static int32_t _printer_gatts_cb(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *param);
static int _match_ble_handle(uint16_t handle);

int printer_server_init(void)
{
    bk_err_t ret = BK_GATT_OK;
    os_printf("\r\n===============================================\r\n");
    os_printf("[BLE_PRN] DN20 BLE Printer Server Initializing...\r\n");
    os_printf("===============================================\r\n");

    printer_protocol_init();
    dm_gatts_add_gatts_callback(_printer_gatts_cb);

    ret = bk_ble_gatts_create_attr_tab((void *)&_printer_attr_db_service, s_gatts_if, PRINTER_HDL_IDX_MAX, 30);
    if (ret != BK_GATT_OK)
    {
        os_printf("[BLE_PRN] Create GATT Attr Tab Failed, ret=%d\r\n", ret);
        return -1;
    }

    return 0;
}

/* 通过 0xFF01 特征值 Notify 发送数据给手机 APP */
int printer_ntf_data(uint8_t *buffer, uint16_t len)
{
    uint16_t handle = _server_printer_attr_handle_list[PRINTER_HDL_IDX_NOTIFY];
    if (handle == INVALID_ATTR_HANDLE || buffer == NULL || len == 0 || gatts_conn_id == 0) 
    {
        return -1;
    }

    os_printf("[BLE_TX -> APP] len=%d: ", len);
    for (uint16_t i = 0; i < len; i++) {
        os_printf("%02X ", buffer[i]);
    }
    os_printf("\r\n");

    /* need_confirm = 0 表示发送标准的 BLE Notification */
    bk_err_t err = bk_ble_gatts_send_indicate(s_gatts_if, (uint16_t)gatts_conn_id, handle, len, buffer, 0);
    if (err != BK_GATT_OK)
    {
        os_printf("[BLE_PRN] send notification failed, err=%d\r\n", err);
        return -1;
    }
    return 0;
}

static int _match_ble_handle(uint16_t handle)
{
    for (int i = 0; i < PRINTER_HDL_IDX_MAX; i++)
    {
        if (handle == _server_printer_attr_handle_list[i])
        {
            return i;
        }
    }
    return -1;
}

static int32_t _printer_gatts_cb(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *param)
{
    switch(event)
    {
        case BK_GATTS_REG_EVT:
            s_gatts_if = gatts_if;
            os_printf("[BLE_PRN] GATTS Registered, gatts_if=%d\r\n", s_gatts_if);
            break;

        case BK_GATTS_CONNECT_EVT:
            gatts_conn_id = param->connect.conn_id;
            os_printf("[BLE_PRN] >>> Phone Connected! conn_id=%d\r\n", gatts_conn_id);
            break;

        case BK_GATTS_DISCONNECT_EVT:
            os_printf("[BLE_PRN] <<< Phone Disconnected! conn_id=%d\r\n", param->disconnect.conn_id);
            if (param->disconnect.conn_id == gatts_conn_id)
            {
                gatts_conn_id = 0;
                printer_protocol_reset();
            }
            break;

        case BK_GATTS_MTU_EVT:
            os_printf("[BLE_PRN] >>> MTU Updated: %d Bytes (Max Payload: %d Bytes)\r\n", 
                      param->mtu.mtu, param->mtu.mtu - 3);
            break;

        case BK_GATTS_CREAT_ATTR_TAB_EVT:
            if(param->add_attr_tab.status == BK_GATT_OK)
            {
                for (uint16_t i = 0; i < param->add_attr_tab.num_handle; i++)
                {
                    _server_printer_attr_handle_list[i] = param->add_attr_tab.handles[i];
                }
                os_printf("[BLE_PRN] Attr Tab Created Successfully. Num Handles=%d\r\n", param->add_attr_tab.num_handle);

                // 启动服务
                bk_ble_gatts_start_service(_server_printer_attr_handle_list[PRINTER_HDL_IDX_SVC]);
            }
            break;

       case BK_GATTS_WRITE_EVT:
        {
            int idx = _match_ble_handle(param->write.handle);
            if(idx != -1)
            {
                bk_gatt_rsp_t rsp;
                os_memset(&rsp, 0, sizeof(rsp));
                // 关键修复：统一将应答 handle 设置为当前写入的属性句柄，避免 handle=0 报错！
                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->write.handle;
                rsp.attr_value.offset = param->write.offset;
                rsp.attr_value.len = 0;

                if(idx == PRINTER_HDL_IDX_WRITE)
                {
                    // 接收到 APP 下发的指令或 BIN 点阵流
                    printer_process_data(param->write.value, param->write.len);

                    if (param->write.need_rsp)
                    {
                        bk_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, BK_GATT_OK, &rsp);
                    }
                }
                else if(idx == PRINTER_HDL_IDX_NOTIFY_DESC || idx == PRINTER_HDL_IDX_NOTIFY_EXTRA_DESC)
                {
                    uint16_t config = (((uint16_t)(param->write.value[1])) << 8) | param->write.value[0];
                    os_printf("[BLE_PRN] CCCD Config: 0x%04X (Notify=%s)\r\n", config, (config & 1) ? "ENABLED" : "DISABLED");
                    
                    // 保存 CCCD 状态
                    if (idx == PRINTER_HDL_IDX_NOTIFY_DESC) {
                        _char_notify_desc_buffer[0] = param->write.value[0];
                        _char_notify_desc_buffer[1] = param->write.value[1];
                    }

                    // 关键修复：必须向手机回复应答，告知通知已开启成功！
                    if (param->write.need_rsp)
                    {
                        bk_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, BK_GATT_OK, &rsp);
                    }
                }
            }
        } break;

        default:
            break;
    }
    return BK_ERR_BT_SUCCESS;
}

