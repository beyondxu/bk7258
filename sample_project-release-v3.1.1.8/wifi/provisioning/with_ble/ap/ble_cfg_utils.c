#include <common/sys_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include "ble_cfg_utils.h"
#include "cJSON.h"
#include <modules/wifi.h>
#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include <modules/wifi.h>
#include <components/event.h>
#include <components/netif.h>
#include <string.h>
#include <os/os.h>
#include "bk_cli.h"
#include "cli.h"


void wifi_boarding_adv_restart(void);
int wifi_boarding_notify(uint8_t *data, uint16_t length);
bk_err_t bk_ble_cfg_send_msg(ble_cfg_msg_t *msg);

static ble_err_t gatt_cmd_status = BK_ERR_BLE_SUCCESS;
static uint8_t gatt_conn_ind = INVALID_HANDLE;

static beken_semaphore_t ble_cmd_sema = NULL;
static uint8_t g_actv_idx = 0xff;
static uint8_t g_ble_inited = 0;
static uint8_t g_ble_cfg_success_cfg = 0;

static beken_queue_t ble_cfg_msg_queue = NULL;
static beken_thread_t ble_netconfig_hdl = NULL;
static wifi_info_t g_wifi_info = {0};

static ble_attm_desc_t gatts_service_ff00_db[GATTS_IDX_FF00_NB] =
{
    //  Service Declaration
    [GATTS_IDX_FF00_SVC]        = {DECL_PRIMARY_SERVICE_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},

    //NTF
    [GATTS_IDX_CHAR_NTF_DECL]  = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    // Characteristic Value
    [GATTS_IDX_CHAR_NTF_VALUE] = {{0xFF02 & 0xFF, 0xFF02 >> 8}, BK_BLE_PERM_SET(NTF, ENABLE)|BK_BLE_PERM_SET(WRITE_REQ, ENABLE)|BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 520},
    //Client Characteristic Configuration Descriptor`
    [GATTS_IDX_CHAR_NTF_DESC] = {DESC_CLIENT_CHAR_CFG_128, BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE), 0, 0},

    //UUID128 WRITE_REQ,RD
    [GATTS_IDX_CHAR_WR_DECL]    = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    [GATTS_IDX_CHAR_WR_VALUE]   = {{0xFF01 & 0xFF, 0xFF01 >> 8}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE) | BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 520},
};

static int ble_netconfig_disconnect_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    BK_EXAMPLE_LOG(TAG,"===disconnect event cb===\n");
    ble_cfg_msg_t msg;

    msg.event = BLE_ACT_CFG_FAIL;
    bk_ble_cfg_send_msg(&msg);

    return BK_OK;
}

static int ble_netconfig_success_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    BK_EXAMPLE_LOG(TAG,"===wifi connect get ip event cb===\n");
    ble_cfg_msg_t msg;

    msg.event = BLE_ACT_CFG_SUCCESS;
    bk_ble_cfg_send_msg(&msg);

    return BK_OK;
}

static void bk_wifi_scan_sort_result(wifi_scan_result_t *scan_result)
{
    if (scan_result->ap_num == 0)
        return;

    for (int i = 0; i < scan_result->ap_num - 1; i++)
    {
        for (int j = 0; j < scan_result->ap_num - i - 1; j++)
        {
            if (scan_result->aps[j].rssi < scan_result->aps[j + 1].rssi)
            {
                wifi_scan_ap_info_t temp = scan_result->aps[j];
                scan_result->aps[j] = scan_result->aps[j + 1];
                scan_result->aps[j + 1] = temp;
            }
        }
    }
}

static int ble_netconfig_scan_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    wifi_scan_result_t scan_result = {0};
    char write_buffer[517] = {0};

    BK_LOG_ON_ERR(bk_wifi_scan_get_result(&scan_result));
    if (scan_result.ap_num == 0)
        goto exit;

    //实现根据rssi 排序
    bk_wifi_scan_sort_result(&scan_result);

    BK_LOG_ON_ERR(bk_wifi_scan_dump_result(&scan_result));

    memset(write_buffer,0,sizeof(write_buffer));
    os_snprintf(write_buffer, sizeof(write_buffer), "[");
    for(int i=0; i<scan_result.ap_num; i++)
    {
        if(os_strlen(write_buffer) + os_strlen(scan_result.aps[i].ssid) + 12 > 500)
        {
            break;
        }
        if(!os_strlen(scan_result.aps[i].ssid))
        {
            continue;
        }
        if(i == 0)
            os_snprintf(write_buffer+os_strlen(write_buffer), sizeof(write_buffer)-os_strlen(write_buffer), "{\"name\":\"%s\"}", scan_result.aps[i].ssid);
        else
            os_snprintf(write_buffer+os_strlen(write_buffer), sizeof(write_buffer)-os_strlen(write_buffer), ",{\"name\":\"%s\"}", scan_result.aps[i].ssid);
    }

    os_snprintf(write_buffer+os_strlen(write_buffer), sizeof(write_buffer)-os_strlen(write_buffer), "]");

exit:
    wifi_boarding_notify((uint8_t *)write_buffer, os_strlen(write_buffer));

    bk_wifi_scan_free_result(&scan_result);

    return BK_OK;
}

static void ble_demo_gatt_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
{
    bk_printf("ble_demo_gatt_cmd_cb: cmd:%d, status:%d\r\n", cmd, param->status);
    gatt_cmd_status = param->status;
    switch (cmd)
    {
        case BLE_CREATE_ADV:
        case BLE_SET_ADV_DATA:
        case BLE_SET_RSP_DATA:
        case BLE_START_ADV:
        case BLE_STOP_ADV:
        case BLE_CREATE_SCAN:
        case BLE_START_SCAN:
        case BLE_STOP_SCAN:
        case BLE_INIT_CREATE:
        case BLE_INIT_START_CONN:
        case BLE_INIT_STOP_CONN:
        case BLE_CONN_DIS_CONN:
        case BLE_CONN_UPDATE_PARAM:
        case BLE_DELETE_ADV:
        case BLE_DELETE_SCAN:
        case BLE_CONN_READ_PHY:
        case BLE_CONN_SET_PHY:
        case BLE_CONN_UPDATE_MTU:
        case BLE_SET_ADV_RANDOM_ADDR:
            if (ble_cmd_sema != NULL)
            {
                rtos_set_semaphore( &ble_cmd_sema );
            }
            break;
        default:
            break;
    }

}

static void ble_gatts_notice_cb(ble_notice_t notice, void *param)
{
    ble_cfg_msg_t msg;
    switch (notice)
    {
        case BLE_5_STACK_OK:
            BK_EXAMPLE_LOG(TAG,"ble stack ok");
            break;
        case BLE_5_WRITE_EVENT:
        {
            ble_write_req_t *w_req = (ble_write_req_t *)param;

            BK_EXAMPLE_LOG(TAG,"BLE_5_WRITE_EVENT write_cb:conn_idx:%d, prf_id:%d, att_idx:%d, len:%d, data[0]:0x%02x\r\n",
                           w_req->conn_idx, w_req->prf_id, w_req->att_idx, w_req->len, w_req->value[0]);

            if(w_req->att_idx == GATTS_IDX_CHAR_NTF_DESC)
            {
                BK_EXAMPLE_LOG(TAG,"write value: %02x %02x", w_req->value[0], w_req->value[1]);
                if(w_req->value[0] == 0x01 && w_req->value[1] == 0x00)
                {
                    bk_printf("cccd enable\r\n");
                    msg.param = (uint32_t*)&g_wifi_info;
                    msg.event = BLE_ACT_WIFI_START_SCAN;
                    bk_ble_cfg_send_msg(&msg);
                }
            }
            else if(w_req->att_idx == GATTS_IDX_CHAR_WR_VALUE)
            {
                BK_EXAMPLE_LOG(TAG,"write value: %s\r\n", w_req->value);
                cJSON* root = cJSON_Parse((const char *) w_req->value);
                if(root == NULL)
                {
                    bk_printf("json parese failed\r\n");
                    goto _out;
                }

                cJSON* ssid = cJSON_GetObjectItem(root, "ssid");
                if(ssid == NULL)
                {
                    bk_printf("ssid find err\r\n");
                    goto _out;
                }

                cJSON* pwd = cJSON_GetObjectItem(root, "pwd");
                if(pwd == NULL)
                {
                    bk_printf("pwd find err\r\n");
                    goto _out;
                }

                bk_printf("ssid:%s,pwd:%s\r\n", ssid->valuestring,pwd->valuestring);
                memset(&g_wifi_info,0,sizeof(g_wifi_info));
                memcpy(g_wifi_info.ssid, ssid->valuestring, strlen(ssid->valuestring));
                memcpy(g_wifi_info.pwd, pwd->valuestring, strlen(pwd->valuestring));

                msg.param = (uint32_t*)&g_wifi_info;
                msg.event = BLE_ACT_WIFI_START_CONNECT;
                bk_ble_cfg_send_msg(&msg);

_out:
                cJSON_Delete(root);

            }

            break;
        }

        case BLE_5_READ_EVENT:
        {
            ble_read_req_t *r_req = (ble_read_req_t *)param;
            char rsp_data[5]="hello";
            BK_EXAMPLE_LOG(TAG,"BLE_5_READ_EVENT read_cb:conn_idx:%d, prf_id:%d, att_idx:%d\r\n",
                           r_req->conn_idx, r_req->prf_id, r_req->att_idx);


            bk_ble_read_response_value(r_req->conn_idx, sizeof(rsp_data), (uint8_t*)rsp_data, r_req->prf_id, r_req->att_idx);
            break;
        }

        case BLE_5_MTU_CHANGE:
        {
            ble_mtu_change_t *m_ind = (ble_mtu_change_t *)param;
            BK_EXAMPLE_LOG(TAG,"BLE_5_MTU_CHANGE %s m_ind:conn_idx:%d, mtu_size:%d\r\n", __func__, m_ind->conn_idx, m_ind->mtu_size);
            break;
        }

        case BLE_5_CONNECT_EVENT:
        {
            ble_conn_ind_t *c_ind = (ble_conn_ind_t *)param;
            BK_EXAMPLE_LOG(TAG,"BLE_5_CONNECT_EVENT c_ind:conn_idx:%d, addr_type:%d, peer_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                           c_ind->conn_idx, c_ind->peer_addr_type, c_ind->peer_addr[0], c_ind->peer_addr[1],
                           c_ind->peer_addr[2], c_ind->peer_addr[3], c_ind->peer_addr[4], c_ind->peer_addr[5]);
            gatt_conn_ind = c_ind->conn_idx;
            break;
        }

        case BLE_5_DISCONNECT_EVENT:
        {
            ble_discon_ind_t *d_ind = (ble_discon_ind_t *)param;
            BK_EXAMPLE_LOG(TAG,"BLE_5_DISCONNECT_EVENT d_ind:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);
            gatt_conn_ind = 0xff;

            if (!g_ble_cfg_success_cfg)
                wifi_boarding_adv_restart();

            break;
        }
        case BLE_5_ATT_INFO_REQ:
        {
            ble_att_info_req_t *a_ind = (ble_att_info_req_t *)param;
            BK_EXAMPLE_LOG(TAG,"BLE_5_ATT_INFO_REQ a_ind:conn_idx:%d\r\n", a_ind->conn_idx);
            a_ind->length = 512;
            a_ind->status = BK_ERR_BLE_SUCCESS;
            break;
        }

        case BLE_5_CREATE_DB:
        {
            ble_create_db_t *cd_ind = (ble_create_db_t *)param;

            BK_EXAMPLE_LOG(TAG,"BLE_5_CREATE_DB cd_ind:prf_id:%d, status:%d\r\n", cd_ind->prf_id, cd_ind->status);
            gatt_cmd_status = cd_ind->status;
            if (ble_cmd_sema != NULL)
            {
                rtos_set_semaphore( &ble_cmd_sema );
            }
            break;
        }

        case BLE_5_READ_PHY_EVENT:
        {
            ble_read_phy_t *phy_param = (ble_read_phy_t *)param;
            BK_EXAMPLE_LOG(TAG,"BLE_5_READ_PHY_EVENT:tx_phy:0x%02x, rx_phy:0x%02x\r\n",phy_param->tx_phy, phy_param->rx_phy);
            break;
        }
        case BLE_5_TX_DONE:
            break;
        case BLE_5_CONN_UPDATA_EVENT:
        {
            ble_conn_param_t *updata_param = (ble_conn_param_t *)param;
            BK_EXAMPLE_LOG(TAG,"BLE_5_CONN_UPDATA_EVENT:conn_interval:0x%04x, con_latency:0x%04x, sup_to:0x%04x\r\n", updata_param->intv_max,
                           updata_param->con_latency, updata_param->sup_to);
            break;
        }
        case BLE_5_PAIRING_REQ:
            BK_EXAMPLE_LOG(TAG,"BLE_5_PAIRING_REQ\r\n");
            break;

        case BLE_5_ENCRYPT_EVENT:
            BK_EXAMPLE_LOG(TAG,"BLE_5_ENCRYPT_EVENT\r\n");
            break;

        case BLE_5_PAIRING_SUCCEED:
            BK_EXAMPLE_LOG(TAG,"BLE_5_PAIRING_SUCCEED\r\n");
            break;

        case BLE_5_KEY_EVENT:
        {
            BK_EXAMPLE_LOG(TAG,"BLE_5_KEY_EVENT\r\n");
            break;
        }
        case BLE_5_BOND_INFO_REQ_EVENT:
        {
            BK_EXAMPLE_LOG(TAG,"BLE_5_BOND_INFO_REQ_EVENT\r\n");
            break;
        }
        default:
            break;
    }
}

static void ble_demo_create_db(void)
{
    int ret = BK_FAIL;
    struct bk_ble_db_cfg ble_db_cfg;

    ble_db_cfg.att_db = (ble_attm_desc_t *)gatts_service_ff00_db;
    ble_db_cfg.att_db_nb = GATTS_IDX_FF00_NB;
    ble_db_cfg.prf_task_id = 0;
    ble_db_cfg.start_hdl = 0;
    ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
    ble_db_cfg.uuid[0] = GATTS_SERVICE_FF00_UUID & 0xFF;
    ble_db_cfg.uuid[1] = GATTS_SERVICE_FF00_UUID >> 8;

    ret = bk_ble_create_db(&ble_db_cfg);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        BK_EXAMPLE_LOG(TAG,"create gatt db failed %d\n", ret);
        return;
    }

    ret = rtos_get_semaphore(&ble_cmd_sema, GATT_SYNC_CMD_TIMEOUT_MS);

    if (ret != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG,"wait semaphore failed at %d, %d\n", ret, __LINE__);
        return;
    }
    else
    {
        BK_EXAMPLE_LOG(TAG,"create gatt db 0xff00 success\n");
    }

}


void wifi_boarding_adv_start(void)
{
    //set adv data
    int ret;
    uint8_t adv_data[251] = {0};
    uint8_t adv_index = 0;
    uint8_t len_index = 0;
    uint8_t mac[6];

    BK_EXAMPLE_LOG(TAG,"%s \n", __func__);

    /* flags */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_FLAGS;
    adv_data[adv_index++] = 0x06;
    adv_data[len_index] = 2;

    /* local name */
    bk_bluetooth_get_address(mac);

    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_LOCAL_NAME;

    ret = sprintf((char *)&adv_data[adv_index], "XZ-%02X%02X%02X", mac[0], mac[1], mac[2]);

    bk_ble_appm_set_dev_name(ret, &adv_data[adv_index]);

    BK_EXAMPLE_LOG(TAG,"%s, dev_name:%s, ret:%d \n", __func__, (char *)&adv_data[adv_index], ret);
    adv_index += ret;
    adv_data[len_index] = ret + 1;

    /* 16bit uuid */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_SERVICE_DATA;
    adv_data[adv_index++] = GATTS_SERVICE_FF00_UUID & 0xFF;
    adv_data[adv_index++] = GATTS_SERVICE_FF00_UUID >> 8;
    adv_data[len_index] = 3;

    /* manufacturer */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_MANUFACTURER_SPECIFIC;
    adv_data[adv_index++] = BEKEN_COMPANY_ID & 0xFF;
    adv_data[adv_index++] = BEKEN_COMPANY_ID >> 8;
    adv_data[len_index] = 3;
    BK_EXAMPLE_LOG(TAG,"adv data length :%d \n", adv_index);

    /* set adv paramters */
    ble_adv_param_t adv_param;

    os_memset(&adv_param, 0, sizeof(ble_adv_param_t));
    adv_param.chnl_map = ADV_ALL_CHNLS;
    adv_param.adv_intv_min = BLE_GATTS_ADV_INTERVAL_MIN;
    adv_param.adv_intv_max = BLE_GATTS_ADV_INTERVAL_MAX;

    adv_param.own_addr_type = OWN_ADDR_TYPE_PUBLIC_ADDR;
    adv_param.adv_type = ADV_TYPE_LEGACY;
    adv_param.adv_prop = ADV_PROP_CONNECTABLE_BIT | ADV_PROP_SCANNABLE_BIT;
    adv_param.prim_phy = INIT_PHY_TYPE_LE_1M;
    adv_param.second_phy = INIT_PHY_TYPE_LE_1M;


    g_actv_idx = bk_ble_get_idle_actv_idx_handle();
    if(g_actv_idx == INVALID_HANDLE)
    {
        BK_EXAMPLE_LOG(TAG,"get idle actv idx failed \n");
        goto error;
    }

    ret = bk_ble_create_advertising(g_actv_idx, &adv_param, ble_demo_gatt_cmd_cb);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        BK_EXAMPLE_LOG(TAG,"config adv paramters failed %d\n", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&ble_cmd_sema, GATT_SYNC_CMD_TIMEOUT_MS);

    if (ret != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG,"wait semaphore failed at %d, %d\n", ret, __LINE__);
        goto error;
    }
    else
    {
        BK_EXAMPLE_LOG(TAG,"set adv paramters success\n");
    }

    /* set adv paramters */
    ret = bk_ble_set_adv_data(g_actv_idx, adv_data, adv_index, ble_demo_gatt_cmd_cb);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        BK_EXAMPLE_LOG(TAG,"set adv data failed %d\n", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&ble_cmd_sema, GATT_SYNC_CMD_TIMEOUT_MS);

    if (ret != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG,"wait semaphore failed at %d, %d\n", ret, __LINE__);
        goto error;
    }
    else
    {
        BK_EXAMPLE_LOG(TAG,"set adv data success\n");
    }

    /* sart adv */
    ret = bk_ble_start_advertising(g_actv_idx, 0, ble_demo_gatt_cmd_cb);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        BK_EXAMPLE_LOG(TAG,"start adv failed %d\n", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&ble_cmd_sema, GATT_SYNC_CMD_TIMEOUT_MS);

    if (ret != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG,"wait semaphore failed at %d, %d\n", ret, __LINE__);
        goto error;
    }
    else
    {
        BK_EXAMPLE_LOG(TAG,"start adv success\n");
    }

    BK_EXAMPLE_LOG(TAG,"%s success\n", __func__);
    return;

error:

    BK_EXAMPLE_LOG(TAG,"%s err\n", __func__);
    return;
}

void wifi_boarding_adv_restart(void)
{
    int err = BK_FAIL;
    BK_EXAMPLE_LOG(TAG,"%s \n", __func__);
    err = bk_ble_start_advertising(g_actv_idx, 0,ble_demo_gatt_cmd_cb);

    if(err != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG,"restart adv fail \n");
    }

    return;
}

void wifi_boarding_adv_stop(void)
{
    int err = BK_FAIL;

    BK_EXAMPLE_LOG(TAG,"%s \n", __func__);
    err = bk_ble_stop_advertising(g_actv_idx, ble_demo_gatt_cmd_cb);

    if(err != BK_OK)
    {
        return;
    }
    err = rtos_get_semaphore(&ble_cmd_sema, GATT_SYNC_CMD_TIMEOUT_MS);
    if (err != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG,"wait semaphore failed at %d, %d\n", err, __LINE__);
    }
    else
    {
        BK_EXAMPLE_LOG(TAG,"start or stop adv success\n");
    }

    err = bk_ble_delete_advertising(g_actv_idx,ble_demo_gatt_cmd_cb);
    if(err != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG,"delete adv fail \n");
    }
    err = rtos_get_semaphore(&ble_cmd_sema, GATT_SYNC_CMD_TIMEOUT_MS);
    if (err != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG,"wait semaphore failed at %d, %d\n", err, __LINE__);
    }
    else
    {
        BK_EXAMPLE_LOG(TAG,"delete adv success\n");
    }

    g_actv_idx = 0xff;
    return;
}

int wifi_boarding_notify(uint8_t *data, uint16_t length)
{
    BK_EXAMPLE_LOG(TAG,"%s \n", __func__);

    if (gatt_conn_ind == 0xFF)
    {
        return BK_FAIL;
    }
    else
    {
        bk_ble_send_noti_value(gatt_conn_ind, length, data,0,GATTS_IDX_CHAR_NTF_VALUE);
        return BK_OK;
    }
}


void ble_config_send_state(uint8_t success_flag)
{
    uint8_t write_buffer[64]= {0};

    BK_EXAMPLE_LOG(TAG,"%s \n", __func__);

    if(success_flag)
    {
        g_ble_cfg_success_cfg = 1;
        memcpy(write_buffer,CONNECT_SUCCESS_MSG,strlen(CONNECT_SUCCESS_MSG));
        wifi_boarding_notify(write_buffer,strlen(CONNECT_SUCCESS_MSG));
    }
    else
    {
        g_ble_cfg_success_cfg = 0;
        memcpy(write_buffer,CONNECT_FAIL_MSG,strlen(CONNECT_FAIL_MSG));
        wifi_boarding_notify(write_buffer,strlen(CONNECT_FAIL_MSG));
    }
}


static void ble_wifi_cfg_thread(beken_thread_arg_t param)
{
    ble_cfg_msg_t msg;
    int ret = 0;

    wifi_boarding_init();
    wifi_boarding_adv_start();

    while (1)
    {

        ret = rtos_pop_from_queue(&ble_cfg_msg_queue, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == ret)
        {
            switch (msg.event)
            {
                case BLE_ACT_WIFI_START_SCAN:
                    BK_LOG_ON_ERR(bk_wifi_scan_start(NULL));
                    break;

                case BLE_ACT_WIFI_START_CONNECT:
                {
                    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();
                    memset(&sta_config.ssid,0,sizeof(sta_config.ssid));
                    memset(&sta_config.password,0,sizeof(sta_config.password));

                    memcpy(sta_config.ssid, ((wifi_info_t*)msg.param)->ssid, strlen(((wifi_info_t*)msg.param)->ssid));
                    memcpy(sta_config.password, ((wifi_info_t*)msg.param)->pwd, strlen(((wifi_info_t*)msg.param)->pwd));

                    BK_EXAMPLE_LOG(TAG,"BLE_ACT_WIFI_START_CONNECT ssid:%s password:%s\n", sta_config.ssid, sta_config.password);
                    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
                    BK_LOG_ON_ERR(bk_wifi_sta_start());
                    break;
                }

                case BLE_ACT_CFG_SUCCESS:
                    ble_config_send_state(1);
                    rtos_delay_milliseconds(40);
                    TEST_PASS();
                    goto exit;
                    break;

                case BLE_ACT_CFG_FAIL:
                    ble_config_send_state(0);
                    TEST_FAIL();
                    break;

                case BLE_ACT_BLE_CONFIG_STOP:
                    goto exit;
                    break;

                default:
                    break;
            }

        }
    }

exit:
    BK_EXAMPLE_LOG(TAG,"%s exit\n", __func__);
    wifi_boarding_deinit();

    rtos_deinit_queue(&ble_cfg_msg_queue);
    ble_cfg_msg_queue = NULL;
    ble_netconfig_hdl = NULL;

    rtos_delete_thread(NULL);
}


int wifi_boarding_init(void)
{
    int ret = BK_FAIL;

    BK_EXAMPLE_LOG(TAG,"%s \n", __func__);

    g_ble_cfg_success_cfg = 0;

    if (g_ble_inited)
        return BK_OK;

    ret = bk_bluetooth_init();
    if(ret != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG,"gatts init fail \n");
        return BK_FAIL;
    }

    if (ble_cmd_sema == NULL)
    {
        ret = rtos_init_semaphore(&ble_cmd_sema, 1);
        if(ret != BK_OK)
        {
            BK_EXAMPLE_LOG(TAG,"gatts init sema fial \n");
            return BK_FAIL;
        }
    }

    bk_ble_set_notice_cb(ble_gatts_notice_cb);
    ble_demo_create_db();
    g_ble_inited = 1;

    return BK_OK;
}

int wifi_boarding_deinit()
{
    BK_EXAMPLE_LOG(TAG,"%s \n", __func__);

    if (!g_ble_inited)
        return BK_OK;

    g_ble_inited = 0;

    bk_event_unregister_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE, ble_netconfig_scan_event_cb);
    bk_event_unregister_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED, ble_netconfig_disconnect_event_cb);
    bk_event_unregister_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4, ble_netconfig_success_event_cb);


    if (gatt_conn_ind != 0xFF)
    {
        bk_ble_disconnect(gatt_conn_ind);
        gatt_conn_ind = 0xFF;
    }

    wifi_boarding_adv_stop();
    bk_ble_set_notice_cb(NULL);
    bk_bluetooth_deinit();
    rtos_deinit_semaphore(&ble_cmd_sema);
    ble_cmd_sema = NULL;

    return BK_OK;
}


bk_err_t bk_ble_cfg_send_msg(ble_cfg_msg_t *msg)
{
    bk_err_t ret = BK_OK;

    if (ble_cfg_msg_queue)
    {
        ret = rtos_push_to_queue(&ble_cfg_msg_queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            BK_LOGE(TAG, "%s failed\n", __func__);
            return BK_FAIL;
        }
    }

    return ret;
}

void ble_network_cfg_start(void)
{
    int ret;

    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE, ble_netconfig_scan_event_cb, NULL);
    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED, ble_netconfig_disconnect_event_cb, NULL);
    bk_event_register_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4, ble_netconfig_success_event_cb, NULL);

    if(!ble_cfg_msg_queue)
    {
        ret = rtos_init_queue(&ble_cfg_msg_queue,
                              "ble_cfg_msg_queue",
                              sizeof(ble_cfg_msg_t),
                              10);
        if(ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, ceate ble_cfg_msg_queue failed\n", __func__);
            return;
        }
    }


    if(!ble_netconfig_hdl)
    {
        ret = rtos_create_thread(&ble_netconfig_hdl,4,"ble_wifi_config task", ble_wifi_cfg_thread,2048,NULL);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, ceate ble_netconfig_hdl failed\n", __func__);
            return;
        }
    }

}

void ble_network_cfg_stop(void)
{
    int ret;
    ble_cfg_msg_t msg;

    msg.event = BLE_ACT_BLE_CONFIG_STOP;
    ret = bk_ble_cfg_send_msg(&msg);
    if (BK_OK != ret)
    {
        BK_LOGE(TAG, "%s, send stop msg failed\n", __func__);
    }
}

void cli_ble_cfg_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (strcmp(argv[1], "stop") == 0)
    {
        ble_network_cfg_stop();
    }
    else if (strcmp(argv[1], "start") == 0)
    {
        ble_network_cfg_start();
    }
    else if (strcmp(argv[1], "adv_stop") == 0)
    {
        wifi_boarding_adv_stop();
    }
    else if (strcmp(argv[1], "adv_start") == 0)
    {
        wifi_boarding_adv_restart();
    }
    else if (strcmp(argv[1], "disconnect") == 0)
    {
        bk_ble_disconnect(gatt_conn_ind);
    }
    else
    {
        BK_EXAMPLE_LOG(TAG,"Usage: %s <stop|start|adv_stop|adv_start|notify>\n", argv[0]);
    }

    return;
}

#define BLE_CFG_CMD_CNT (sizeof(s_ble_cfg_commands) / sizeof(struct cli_command))
static const struct cli_command s_ble_cfg_commands[] =
{
    {"ble_cfg", "ble_cfg", cli_ble_cfg_cmd},
};

int cli_ble_cfg_init(void)
{
    return cli_register_commands(s_ble_cfg_commands, BLE_CFG_CMD_CNT);
}