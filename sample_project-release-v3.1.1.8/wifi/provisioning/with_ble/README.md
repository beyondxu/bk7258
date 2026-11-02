# BLE WiFi配网示例

| 支持的芯片 | BK7258 | BK7257 |
| --------- | ------- | ------- |


## 概述

本示例基于 BLE（蓝牙低功耗）和 Wi-Fi 组件，演示了如何通过 BLE 进行 Wi-Fi 网络配置。它具体展示了如何：

1. 配置并启动 BLE GATT 服务
2. 通过 BLE 接收 Wi-Fi SSID 和密码等配网信息
3. 使用接收到的信息连接到指定的 Wi-Fi 网络
4. 通过 BLE 通知客户端配网结果

## 相关配置

以下配置可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_BLE          | BLE功能使能配置，配置为y                                         |
| CONFIG_CJSON_USE    | JSON 功能使能配置，本示例配置为y|


## 核心配置文件（ble_cfg_utils.h）

ble_cfg_utils.h 文件包含了项目的核心配置和接口定义：

1. **BLE 配置**：
   - GATT 服务 UUID 和特征值定义
   - BLE 广播间隔配置
   - BLE 事件类型定义

2. **Wi-Fi 配置**：
   - `wifi_info_t` 结构体：包含 SSID 和密码字段

3. **统一日志接口**：
   - `BK_EXAMPLE_LOG(tag, format, ...)`：示例日志接口
   - `TAG`：日志标签（"ble_netconfig"）
   - `TEST_PASS()`：测试通过日志
   - `TEST_FAIL()`：测试失败日志

4. **主要接口函数**：
   - `wifi_boarding_init()`：初始化 Wi-Fi 配网模块
   - `wifi_boarding_deinit()`：反初始化 Wi-Fi 配网模块
   - `wifi_boarding_adv_start()`：启动 BLE 广播
   - `wifi_boarding_adv_stop()`：停止 BLE 广播
   - `wifi_boarding_notify()`：通过 BLE 发送通知
   - `ble_network_cfg_start()`：开始 BLE 网络配置
   - `ble_network_cfg_stop()`：停止 BLE 网络配置

## 常用接口

### BLE 接口
更多接口请查看 bk_ble.h 和 bk_dm_bluetooth.h。

``` C
    // BLE GATT 服务描述符定义
    typedef struct {
        uint8_t uuid[16];
        uint8_t perm;
        uint8_t max_length;
        uint16_t length;
    } ble_attm_desc_t;

    // 发送 BLE 通知
    int wifi_boarding_notify(uint8_t *data, uint16_t length);

    // 启动 BLE 广播
    void wifi_boarding_adv_start(void);

    // 停止 BLE 广播
    void wifi_boarding_adv_stop(void);
```

### Wi-Fi 接口
更多接口请查看 wifi.h。

``` C
    // 配置 STA 参数
    bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t *sta_config);

    // 启动 STA
    bk_err_t bk_wifi_sta_start(void);

    // 获取 STA 连接状态
    bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status);

    // 停止 STA
    bk_err_t bk_wifi_sta_stop(void);
```

## 相关代码

### 主程序入口（ap_main.c）

``` C
int main(void)
{
    bk_init();

    cli_ble_cfg_init();
    ble_network_cfg_start();
    
    return 0;
}
```

### BLE 网络配置启动函数（ble_cfg_utils.c）

``` C
void ble_network_cfg_start(void)
{
    int ret = 0;
    BK_EXAMPLE_LOG(TAG, "ble network config start\r\n");

    ret = wifi_boarding_init();
    if (ret) {
        BK_EXAMPLE_LOG(TAG, "wifi boarding init failed ret %d\r\n", ret);
        return;
    }

    wifi_boarding_adv_start();
}
```

### BLE GATT 服务定义（ble_cfg_utils.c）

``` C
static ble_attm_desc_t gatts_service_ff00_db[GATTS_IDX_FF00_NB] = {
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
```

## 操作说明
### 编译

编译命令如下：

```bash
cd sample_project/wifi/provisioning/with_ble
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258  // 或 make bk7257，根据目标芯片选择
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录build文件夹里的all-app.bin;
- 烧录完成后，板子会自动启动并开启 BLE 广播;
- 使用手机微信扫描二维码(sample_project/wifi/provisioning/with_ble/QR.jpg),搜索并连接到板子的 BLE 设备;
- 通过 BLE GATT 服务向板子发送 Wi-Fi SSID 和密码;
- 板子接收到配网信息后，会尝试连接到指定的 Wi-Fi 网络;
- 连接结果会通过 BLE 通知发送回客户端;


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。