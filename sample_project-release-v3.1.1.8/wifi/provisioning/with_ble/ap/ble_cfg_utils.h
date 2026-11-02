#ifndef __WIFI_BOARDING_H__
#define __WIFI_BOARDING_H__

#define ADV_TYPE_FLAGS                      (0x01)
#define ADV_TYPE_LOCAL_NAME                 (0x09)
#define ADV_TYPE_SERVICE_UUIDS_16BIT        (0x14)
#define ADV_TYPE_SERVICE_DATA               (0x16)
#define ADV_TYPE_MANUFACTURER_SPECIFIC      (0xFF)
#define BEKEN_COMPANY_ID                    (0x05F0)

#define GATT_SYNC_CMD_TIMEOUT_MS            4000
#define INVALID_HANDLE                      0xFF

#define GATTS_SERVICE_FF00_UUID 			(0xFF00)

#define DECL_PRIMARY_SERVICE_128            {0x00,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define DECL_CHARACTERISTIC_128             {0x03,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define DESC_CLIENT_CHAR_CFG_128            {0x02,0x29,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

#define BLE_GATTS_ADV_INTERVAL_MIN          120
#define BLE_GATTS_ADV_INTERVAL_MAX          160
#define BLE_CCCD_ATT_MAX                    6

#define BK_EXAMPLE_LOG(tag, format, ...)             bk_printf("[%s]:" format, tag, ##__VA_ARGS__)
#define TAG "ble_netconfig"
#define TEST_PASS() 	BK_EXAMPLE_LOG(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL() 	BK_EXAMPLE_LOG(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")



enum
{
    GATTS_IDX_FF00_SVC,

    GATTS_IDX_CHAR_NTF_DECL,
    GATTS_IDX_CHAR_NTF_VALUE,
    GATTS_IDX_CHAR_NTF_DESC,


    GATTS_IDX_CHAR_WR_DECL,
    GATTS_IDX_CHAR_WR_VALUE,

    GATTS_IDX_FF00_NB,
};

typedef struct
{
    char ssid[33];
    char pwd[65];
} wifi_info_t;

typedef enum
{
    BLE_ACT_WIFI_START_CONNECT,
    BLE_ACT_WIFI_START_SCAN,
    BLE_ACT_CFG_SUCCESS,
    BLE_ACT_CFG_FAIL,
    BLE_ACT_BLE_CONFIG_STOP,
    BLE_ACT_MES_MAX
} BLE_WIFI_CFG_MSG_TYPE;

typedef struct
{
    uint32_t event;
    uint32_t *param;
} ble_cfg_msg_t;


#define CONNECT_SUCCESS_MSG "{\"state\":\"success\"}"
#define CONNECT_FAIL_MSG "{\"state\":\"fail\"}"

#define INVALID_ATTR_HANDLE 0
#define SYNC_CMD_TIMEOUT_MS 4000
#define ADV_HANDLE 0


int wifi_boarding_init(void);
int wifi_boarding_deinit();
void wifi_boarding_adv_start(void);
void wifi_boarding_adv_stop(void);
int wifi_boarding_notify(uint8_t *data, uint16_t length);
void ble_config_send_state(uint8_t success_flag);
int cli_ble_cfg_init(void);
void ble_network_cfg_start(void);
void ble_network_cfg_stop(void);
#endif

