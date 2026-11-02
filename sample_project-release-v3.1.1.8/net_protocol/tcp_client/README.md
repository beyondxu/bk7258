# TCP客户端示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 Wi-Fi 组件和 TCP 协议，演示了一个简单的 TCP 客户端实现。它具体展示了如何：

1. 配置 Wi-Fi STA 参数，启动 STA 功能并连接到指定的 Wi-Fi 网络
2. 使用 os.h rtos_create_thread 接口创建线程
3. 使用 lwIP 网络栈实现 TCP 客户端功能
4. 连接到指定的 TCP 服务器，发送初始消息并循环接收/回复数据

## 项目结构

```
tcp_client/
├── ap/
│   ├── ap_main.c          # 主程序入口
│   ├── wifi_manager.c     # Wi-Fi管理模块
│   ├── wifi_manager.h     # Wi-Fi管理模块头文件
│   ├── tcp_client.c       # TCP客户端实现
│   ├── tcp_client.h       # TCP客户端头文件
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

2. **TCP客户端配置**：
   - `TCP_SERVER_IP`：TCP服务器IP地址（默认为"192.168.2.37"）
   - `TCP_SERVER_PORT`：TCP服务器端口（默认为8888）
   - `TCP_BUFFER_SIZE`：TCP数据缓冲区大小（默认为1024字节）

3. **统一日志接口**：
   - `TCP_TAG`：日志标签（"tcp_client"）
   - `TCP_LOGI(...)`：信息日志
   - `TCP_LOGE(...)`：错误日志
   - `TCP_LOGD(...)`：调试日志
   - `TCP_LOGV(...)`：详细日志

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

### TCP接口
更多接口请查看lwip/sockets.h、lwip/inet.h。

``` C
    //创建套接字
    int socket(int domain, int type, int protocol);

    //连接服务器
    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

    //发送数据
    ssize_t send(int sockfd, const void *buf, size_t len, int flags);

    //接收数据
    ssize_t recv(int sockfd, void *buf, size_t len, int flags);

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
            TCP_LOGE("Failed to connect to WiFi: %d\n", ret);
            break;
        }

        // 创建TCP客户端线程
        ret = tcp_client_create_thread();
        if (ret != BK_OK)
        {
            TCP_LOGE("create tcp client thread failed: %d\n", ret);
            break;
        }
        TCP_LOGI("create tcp client thread success\n");
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
            TCP_LOGE("Invalid WiFi credentials\n");
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
            TCP_LOGE("Failed to set WiFi config: %d\n", ret);
            break;
        }

        // 注册WiFi事件回调
        ret = wifi_register_events();
        if (ret != BK_OK)
        {
            TCP_LOGE("Failed to register WiFi events: %d\n", ret);
            break;
        }

        // 启动WiFi STA模式
        ret = bk_wifi_sta_start();
        if (ret != BK_OK)
        {
            TCP_LOGE("Failed to start WiFi STA: %d\n", ret);
            break;
        }

        TCP_LOGI("WiFi connecting to SSID: %s\n", sta_config.ssid);
    } while (0);
    
    return ret;
}
```

### TCP客户端线程创建（tcp_client.c）
``` C
bk_err_t tcp_client_create_thread(void)
{
    beken_thread_t tcp_thread;
    bk_err_t ret = BK_OK;
    
    do {
        // 创建TCP客户端线程
        ret = rtos_create_thread(&tcp_thread, BEKEN_APPLICATION_PRIORITY, "tcp_client", 
                               tcp_client_thread, 4096, NULL);
        if (ret != BK_OK)
        {
            TCP_LOGE("create tcp client thread failed: %d\n", ret);
            break;
        }
        TCP_LOGI("create tcp client thread success\n");
    } while (0);

    return ret;
}
```

### TCP客户端实现（tcp_client.c）
``` C
void tcp_client_thread(void *arg)
{
    int sockfd = -1;
    struct sockaddr_in server_addr;
    char buffer[TCP_BUFFER_SIZE];
    int ret;

    TCP_LOGI("TCP client thread started\n");

    // 等待网络连接稳定
    while(1)
    {
        if(wifi_sta_is_link_success()) break;
        rtos_delay_milliseconds(100);
    }

    // 创建TCP套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        TCP_LOGE("socket create failed: %d\n", sockfd);
        goto exit;
    }
    TCP_LOGI("socket create success\n");

    // 设置服务器地址
    os_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_SERVER_PORT);
    if (inet_pton(AF_INET, TCP_SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        TCP_LOGE("inet_pton failed\n");
        goto exit;
    }

    // 连接服务器
    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        TCP_LOGE("connect failed: %d\n", ret);
        goto exit;
    }
    TCP_LOGI("connect to server %s:%d success\n", TCP_SERVER_IP, TCP_SERVER_PORT);

    // 发送初始消息
    const char *init_msg = "hello, this is beken tcp client";
    ret = send(sockfd, init_msg, os_strlen(init_msg), 0);
    if (ret < 0)
    {
        TCP_LOGE("send failed: %d\n", ret);
        goto exit;
    }
    TCP_LOGI("send init message success: %s\n", init_msg);

    // 循环接收并回复数据
    while (1)
    {
        // 接收服务器数据
        ret = recv(sockfd, buffer, TCP_BUFFER_SIZE - 1, 0);
        if (ret <= 0)
        {
            if (ret == 0)
            {
                TCP_LOGI("server closed connection\n");
            }
            else
            {
                TCP_LOGE("recv failed: %d\n", ret);
            }
            break;
        }

        // 添加字符串结束符
        buffer[ret] = '\0';
        TCP_LOGI("received from server: %s\n", buffer);

        // 直接回复收到的数据
        ret = send(sockfd, buffer, ret, 0);
        if (ret < 0)
        {
            TCP_LOGE("send reply failed: %d\n", ret);
            break;
        }
        TCP_LOGI("send reply success\n");
    }

exit:
    if (sockfd >= 0)
    {
        close(sockfd);
        TCP_LOGI("socket closed\n");
    }
    rtos_delete_thread(NULL); // 删除当前线程
}
```

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/net_protocol/tcp_client
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录build文件夹里的all-app.bin;
- 确保"sample_project_ap"热点已启动，且密码为"12345678"；
- 确保TCP服务器在192.168.2.37:8888端口监听；
- 板子上电运行后，板子会自动连接"sample_project_ap"的热点，连接成功后会创建TCP客户端线程；
- TCP客户端会连接到指定的服务器，发送初始消息并循环接收/回复数据。

以下是上电运行后的主要log输出：

```
[2025-12-30 14:02:19.851]  ap0:cli:W(1181):BK STA got ip 
[2025-12-30 14:02:19.851]  ap0:tcp_clie:I(1181):STA got ip:192.168.2.35  //连接wifi成功，获取ip
[2025-12-30 14:02:19.851]  ap0:tcp_clie:I(1227):socket create success
[2025-12-30 14:02:20.270]  ap1:tcp_clie:I(1600):connect to server 192.168.2.37:8888 success  //连接tcp服务器成功    
[2025-12-30 14:02:20.270]  ap1:tcp_clie:I(1601):send init message success: hello, this is beken tcp client  //发送数据给tcp服务器
[2025-12-30 14:02:24.373]  ap1:tcp_clie:I(5693):received from server: rfcali_show_data  //接收tcp服务器数据

```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
