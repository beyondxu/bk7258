# HTTP WebServer WiFi配网示例

| 支持的芯片 | BK7258 | BK7257 |
| --------- | ------- | ------- |


## 概述

本示例基于 HTTP WebServer 和 Wi-Fi 组件，演示了如何通过 Web 页面进行 Wi-Fi 网络配置。它具体展示了如何：

1. 配置并启动 Wi-Fi AP 模式
2. 实现一个轻量级 HTTP WebServer
3. 提供 Web 页面供用户输入 Wi-Fi SSID 和密码
4. 使用接收到的信息连接到指定的 Wi-Fi 网络
5. 在 Web 页面上显示连接状态

## 相关配置

以下配置可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_WIFI          | WiFi功能使能配置，配置为y                                         |
| CONFIG_LWIP          | LWIP网络协议栈使能配置，配置为y|                


## 核心配置文件（apconfig_example.h）

apconfig_example.h 文件包含了项目的核心配置和接口定义：

1. **WebServer 配置**：
   - 默认端口号定义（80）
   - 根路径配置
   - HTTP 请求处理函数声明

2. **Wi-Fi 配置**：
   - 默认 AP 名称和密码
   - 默认 AP IP 地址
   - Wi-Fi 连接状态枚举

3. **统一日志接口**：
   - `BK_EXAMPLE_LOG(tag, format, ...)`：示例日志接口
   - `TAG`：日志标签（"web_config"）


4. **主要接口函数**：
   - `ap_config_example_init()`：初始化 AP 配置示例
   - `wifi_connect_to_ap()`：连接到指定的 Wi-Fi 网络
   - `get_wifi_connect_state()`：获取 Wi-Fi 连接状态
   - `wifi_provisioning_web_config_start()`：启动 Web 配网服务

## 常用接口

### WebServer 接口
更多接口请查看 webnet.h 和 wn_module.h。

``` C
    // WebServer初始化
    int webnet_init(void);
    
    // WebServer去初始化
    void webnet_deinit(void);
    
    // 设置WebServer端口
    void webnet_set_port(int port);
    
    // 设置WebServer根路径
    void webnet_set_root(const char* webroot_path);
    
    // 注册HTTP请求处理函数
    int webnet_register_handler(const char* path, webnet_handler_t handler);
    
    // 发送HTTP响应
    int webnet_send_response(struct web_session* session, int status, const char* content_type, const char* content);
```

### Wi-Fi 接口
更多接口请查看 wifi.h。

``` C
    // 配置并启动AP模式
    bk_err_t bk_wifi_ap_start(const char* ssid, const char* password, const char* ip);
    
    // 配置STA参数
    bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t *sta_config);
    
    // 启动STA
    bk_err_t bk_wifi_sta_start(void);
    
    // 获取STA连接状态
    bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status);
    
    // 停止STA
    bk_err_t bk_wifi_sta_stop(void);
    
    // 开始Wi-Fi扫描
    bk_err_t bk_wifi_scan_start(wifi_scan_result_callback_t callback);
```

### Web配网服务启动函数

``` C
void wifi_provisioning_web_config_start(void)
{
    wifi_event_handler_init();
    wifi_softap_init();
    captive_dns_init();

    #ifdef WEBNET_USING_CGI
    webnet_cgi_set_root("/");
    webnet_cgi_register("", cgi_web_ap_config_handler);
    webnet_cgi_register("scan", cgi_web_scan_handler);
    webnet_cgi_register("status", cgi_web_status_handler);
    #endif

    webnet_init();
}
```

## 操作说明
### 编译

编译命令如下：

```bash
cd sample_project/wifi/provisioning/with_http_webserver
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258  // 或 make bk7257，根据目标芯片选择
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录build文件夹里的all-app.bin;
- 烧录完成后，板子会自动启动并创建WiFi热点（默认名称：BK72XX-XXXX，密码：12345678）;
- 使用手机或电脑连接到该WiFi热点;
- 在浏览器中访问 `http://192.168.1.1`;
- 在Web页面上选择要连接的WiFi网络并输入密码;
- 点击"连接"按钮，设备会尝试连接到指定的WiFi网络;
- 页面会显示连接状态和进度;


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。