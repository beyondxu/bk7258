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

#define CHAR_BUFFER_SIZE            512
#define INVALID_ATTR_HANDLE         0

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

static uint8_t s_gatts_if = 1;
static uint16_t gatts_conn_id = 0;
static bool s_is_connected = false;
static bool s_service_inited = false;
static uint16_t _server_printer_attr_handle_list[PRINTER_HDL_IDX_MAX] = {0};

static const bk_gatts_attr_db_t _printer_attr_db_service[] =
{
    { BK_GATT_PRIMARY_SERVICE_DECL_128(PRINTER_SVC_UUID_128), },
    {
        BK_GATT_CHAR_DECL_128(PRINTER_NOTIFY_UUID_128,
                              CHAR_BUFFER_SIZE, _char_notify_buffer,
                              BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                              BK_GATT_PERM_READ,
                              BK_GATT_RSP_BY_APP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(_char_notify_desc_buffer), _char_notify_desc_buffer,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_RSP_BY_APP),
    },
    {
        BK_GATT_CHAR_DECL_128(PRINTER_WRITE_UUID_128,
                              CHAR_BUFFER_SIZE, _char_write_buffer,
                              BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_WRITE_NR,
                              BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                              BK_GATT_RSP_BY_APP),
    },
    {
        BK_GATT_CHAR_DECL_128(PRINTER_NOTIFY_EXTRA_UUID_128,
                              CHAR_BUFFER_SIZE, _char_notify_buffer,
                              BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                              BK_GATT_PERM_READ,
                              BK_GATT_RSP_BY_APP),
    },
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
    if (s_service_inited) return 0;
    s_service_inited = true;

    printer_protocol_init();
    dm_gatts_add_gatts_callback(_printer_gatts_cb);

    return bk_ble_gatts_create_attr_tab((void *)&_printer_attr_db_service, s_gatts_if, PRINTER_HDL_IDX_MAX, 30);
}

int printer_ntf_data(uint8_t *buffer, uint16_t len)
{
    uint16_t notify_hdl = _server_printer_attr_handle_list[PRINTER_HDL_IDX_NOTIFY];
    if (notify_hdl == INVALID_ATTR_HANDLE || buffer == NULL || len == 0 || !s_is_connected) {
        return -1;
    }
    
    // 优先采用标准 notify 发送 (快速、不等待 ACK，避免 APP 阻塞状态)
    // 如果 SDK 支持指示，也可采用 indicate
    return bk_ble_gatts_send_indicate(s_gatts_if, (uint16_t)gatts_conn_id, notify_hdl, len, buffer, 0);
}


static int _match_ble_handle(uint16_t handle)
{
    uint16_t write_hdl  = _server_printer_attr_handle_list[PRINTER_HDL_IDX_WRITE];
    uint16_t notify_hdl = _server_printer_attr_handle_list[PRINTER_HDL_IDX_NOTIFY];
    uint16_t cccd_hdl   = _server_printer_attr_handle_list[PRINTER_HDL_IDX_NOTIFY_DESC];

    if (handle == write_hdl || (write_hdl != 0 && handle == write_hdl + 1)) {
        return PRINTER_HDL_IDX_WRITE;
    }
    if (handle == cccd_hdl || (notify_hdl != 0 && (handle == notify_hdl + 1 || handle == notify_hdl + 2))) {
        return PRINTER_HDL_IDX_NOTIFY_DESC;
    }
    for (int i = 0; i < PRINTER_HDL_IDX_MAX; i++) {
        if (handle == _server_printer_attr_handle_list[i]) return i;
    }
    return -1;
}

static int32_t _printer_gatts_cb(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *param)
{
    if (gatts_if != 0) s_gatts_if = gatts_if;

    switch(event)
    {
        case BK_GATTS_CONNECT_EVT:
            gatts_conn_id = param->connect.conn_id;
            s_is_connected = true;
            os_printf("[BLE_PRN] >>> Phone Connected! conn_id=%d\r\n", gatts_conn_id);
            break;

        case BK_GATTS_DISCONNECT_EVT:
            os_printf("[BLE_PRN] <<< Phone Disconnected!\r\n");
            if (param->disconnect.conn_id == gatts_conn_id) {
                s_is_connected = false;
                printer_protocol_reset();
            }
            break;

        case BK_GATTS_MTU_EVT:
            os_printf("[BLE_PRN] MTU Updated: %d\r\n", param->mtu.mtu);
            break;

        case BK_GATTS_CREAT_ATTR_TAB_EVT:
            if(param->add_attr_tab.status == BK_GATT_OK) {
                for (uint16_t i = 0; i < param->add_attr_tab.num_handle && i < PRINTER_HDL_IDX_MAX; i++) {
                    _server_printer_attr_handle_list[i] = param->add_attr_tab.handles[i];
                }
                bk_ble_gatts_start_service(_server_printer_attr_handle_list[PRINTER_HDL_IDX_SVC]);
            }
            break;

        case BK_GATTS_WRITE_EVT:
        {
            int idx = _match_ble_handle(param->write.handle);

            bk_gatt_rsp_t rsp;
            os_memset(&rsp, 0, sizeof(rsp));
            rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
            rsp.attr_value.handle = param->write.handle;

            if (idx == PRINTER_HDL_IDX_WRITE) {
                // ★ 极速送入协议处理器，严禁在此执行任何耗时串口打印 ★
                printer_process_data(param->write.value, param->write.len);
                if (param->write.need_rsp) {
                    bk_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, BK_GATT_OK, &rsp);
                }
            }
            else if (idx == PRINTER_HDL_IDX_NOTIFY_DESC || idx == PRINTER_HDL_IDX_NOTIFY_EXTRA_DESC) {
                uint16_t config = (((uint16_t)(param->write.value[1])) << 8) | param->write.value[0];
                if (idx == PRINTER_HDL_IDX_NOTIFY_DESC) {
                    _char_notify_desc_buffer[0] = param->write.value[0];
                    _char_notify_desc_buffer[1] = param->write.value[1];
                }
                if (param->write.need_rsp) {
                    bk_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, BK_GATT_OK, &rsp);
                }
                if (config & 1) {
                    printer_report_status();
                }
            }
            else {
                if (param->write.need_rsp) {
                    bk_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, BK_GATT_OK, &rsp);
                }
            }
        } break;

        default:
            break;
    }
    return BK_ERR_BT_SUCCESS;
}

