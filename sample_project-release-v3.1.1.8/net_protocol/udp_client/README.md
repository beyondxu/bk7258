# UDP客户端示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 Wi-Fi 组件和 UDP 协议，演示了一个简单的 UDP 客户端实现。它具体展示了如何：

1. 配置 Wi-Fi STA 参数，启动 STA 功能并连接到指定的 Wi-Fi 网络
2. 使用 os.h rtos_create_thread 接口创建线程
3. 使用 lwIP 网络栈实现 UDP 客户端功能
4. 向指定的 UDP 服务器循环发送数据

## 项目结构

```
udp_client/
├── ap/
│   ├── ap_main.c          # 主程序入口
│   ├── wifi_manager.c     # Wi-Fi管理模块
│   ├── wifi_manager.h     # Wi-Fi管理模块头文件
│   ├── udp_client.c       # UDP客户端实现
│   ├── udp_client.h       # UDP客户端头文件
│   └── common.h           # 公共配置和日志接口
├── cp/
├── partitions/
├── build/
├── CMakeLists.txt
├── Makefile
└── README.md
```

## 公共配置文件（common.h）

common.h 文件包含了项目的公共配置和统一的日志接口：

1. **Wi-Fi配置**：
   - `WIFI_AP_SSID`：要连接的Wi-Fi网络名称（默认为"sample_project_ap"）
   - `WIFI_AP_PASSWORD`：Wi-Fi网络密码（默认为"12345678"）

2. **UDP客户端配置**：
   - `UDP_SERVER_IP`：UDP服务器IP地址（默认为"192.168.2.37"）
   - `UDP_SERVER_PORT`：UDP服务器端口（默认为8888）
   - `UDP_BUFFER_SIZE`：UDP数据缓冲区大小（默认为1024字节）

3. **统一日志接口**：
   - `UDP_TAG`：日志标签（"udp_client"）
   - `UDP_LOGI(...)`：信息日志
   - `UDP_LOGE(...)`：错误日志
   - `UDP_LOGD(...)`：调试日志
   - `UDP_LOGV(...)`：详细日志

所有模块都应包含此文件以使用统一的配置和日志接口。

## 常用接口

### Wi-Fi接口
更多接口请查看wifi.h。

``` C
    //配置STA参数
    bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t *sta_config);

    //启动sta
    bk_err_t bk_wifi_sta_start(void);

    //获取sta连接状态、连接的ap、rssi等信息
    bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status);

    //停止sta
    bk_err_t bk_wifi_sta_stop(void);

    //注册wi-fi事件回调函数
    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_sta_event_cb, NULL);

    //注册网络事件回调函数
    bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL);
```

### RTOS接口
更多接口请查看os.h。

``` C
    //创建线程
    bk_err_t rtos_create_thread(beken_thread_t *thread, uint8_t priority, const char *name, 
                               beken_thread_function_t function, uint32_t stack_size, 
                               beken_thread_arg_t arg);

    //删除线程
    bk_err_t rtos_delete_thread(beken_thread_t *thread);

    //延迟函数
    void rtos_delay_milliseconds(uint32_t ms);
```

### UDP接口
更多接口请查看lwip/sockets.h、lwip/inet.h。

``` C
    //创建套接字
    int socket(int domain, int type, int protocol);

    //发送数据
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen);

    //接收数据
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen);

    //关闭套接字
    int close(int fd);
```

## 相关代码

### 主程序入口（ap_main.c）
``` C
int main(void)
{
    int ret = 0;

    bk_init();
    
    do {
        // 初始化Wi-Fi STA模式并尝试连接
        ret = wifi_sta_connect(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to connect to WiFi: %d\n", ret);
            break;
        }

        // 创建UDP客户端线程
        ret = udp_client_create_thread();
        if (ret != BK_OK)
        {
            UDP_LOGE("create udp client thread failed: %d\n", ret);
            break;
        }
        UDP_LOGI("create udp client thread success\n");
    } while (0);

    return ret;
}
```

### Wi-Fi STA初始化（wifi_manager.c）
``` C
int wifi_sta_connect(const char *ssid, const char *password)
{
    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();
    int ret = BK_OK;

    do {
        // 参数有效性检查
        if (ssid == NULL || password == NULL)
        {
            UDP_LOGE("Invalid WiFi credentials\n");
            ret = BK_ERR_PARAM;
            break;
        }

        // 设置WiFi SSID和密码
        os_memset(sta_config.ssid, 0, sizeof(sta_config.ssid));
        os_strncpy(sta_config.ssid, ssid, os_strlen(ssid));
        os_memset(sta_config.password, 0, sizeof(sta_config.password));
        os_strncpy(sta_config.password, password, os_strlen(password));

        // 设置WiFi配置
        ret = bk_wifi_sta_set_config(&sta_config);
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to set WiFi config: %d\n", ret);
            break;
        }

        // 注册WiFi事件回调
        ret = wifi_register_events();
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to register WiFi events: %d\n", ret);
            break;
        }

        // 启动WiFi STA模式
        ret = bk_wifi_sta_start();
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to start WiFi STA: %d\n", ret);
            break;
        }

        UDP_LOGI("WiFi connecting to SSID: %s\n", sta_config.ssid);
    } while (0);
    
    return ret;
}
```

### UDP客户端线程（udp_client.c）
``` C
void udp_client_thread(void *arg)
{
    int sockfd = -1;
    struct sockaddr_in server_addr;
    int ret;
    int count = 0;

    UDP_LOGI("UDP client thread started\n");

    // 等待网络连接稳定
    while(1)
    {
        if(wifi_sta_is_link_success()) break;
        rtos_delay_milliseconds(100);
    }

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        UDP_LOGE("socket create failed: %d\n", sockfd);
        goto exit;
    }
    UDP_LOGI("socket create success\n");

    // 设置服务器地址
    os_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_SERVER_PORT);
    if (inet_pton(AF_INET, UDP_SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        UDP_LOGE("inet_pton failed\n");
        goto exit;
    }

    UDP_LOGI("sending to server %s:%d\n", UDP_SERVER_IP, UDP_SERVER_PORT);

    // 循环发送数据到服务器
    while (1)
    {
        char msg[128];
        os_sprintf(msg, "hello from udp client, count=%d", count++);
        
        ret = sendto(sockfd, msg, os_strlen(msg), 0, 
                     (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0)
        {
            UDP_LOGE("sendto failed: %d\n", ret);
            rtos_delay_milliseconds(100);
            continue;
        }
        
        UDP_LOGI("send success: %s\n", msg);
        TEST_PASS();
        
        // 延迟1秒后继续发送
        rtos_delay_milliseconds(1000);
    }

exit:
    if (sockfd >= 0)
    {
        close(sockfd);
        UDP_LOGI("socket closed\n");
    }
    rtos_delete_thread(NULL);
}
```

## 操作说明
### 编译

编译命令如下：

```bash
cd sample_project/net_protocol/udp_client
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录build文件夹里的all-app.bin;
- 确保"sample_project_ap"热点已启动，且密码为"12345678"；
- 确保UDP服务器在192.168.50.207:8888端口监听(PC端可以使用sscom32等工具)；
- 板子上电运行后，板子会自动连接"sample_project_ap"的热点，连接成功后会创建UDP客户端线程；
- UDP客户端每隔一秒收到板子发送的数据。

以下是上电运行后的关键串口日志：

```
[2025-12-30 15:42:48.379]  ap0:cli:W(1556):BK STA connected E0_xiao
[2025-12-30 15:42:48.379]  ap0:udp_clie:I(1556):STA connected to E0_xiao
[2025-12-30 15:42:48.379]  ap0:cli:W(1556):BK STA got ip
[2025-12-30 15:42:48.379]  ap0:udp_clie:I(1556):STA got ip:192.168.50.202   //wifi连接成功
[2025-12-30 15:42:48.380]  ap0:udp_clie:I(1624):socket create success
[2025-12-30 15:42:48.488]  ap0:udp_clie:I(1624):sending to server 192.168.50.207:8888
[2025-12-30 15:42:48.488]  ap0:udp_clie:I(1625):send success: hello from udp client, count=0  //发送数据给udp服务器
[2025-12-30 15:42:48.488]  ap0:udp_clie:I(1625):/homeb/honghui.xiao/7258/avdk_smp/sample_project/net_protocol/udp_client/ap/udp_client.c:76:udp_client_thread:PASS
```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。