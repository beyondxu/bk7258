# TCP服务器示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 Wi-Fi 组件和 TCP 协议，演示了一个简单的 TCP 服务器实现。它具体展示了如何：

1. 配置 Wi-Fi STA 参数，连接到 AP
2. 使用 os.h rtos_create_thread 接口创建线程
3. 使用 lwIP 网络栈实现 TCP 服务器功能
4. 监听指定端口，接受客户端连接并循环接收/回复数据

## 项目结构

```
tcp_server/
├── ap/
│   ├── ap_main.c          # 主程序入口
│   ├── wifi_manager.c     # Wi-Fi管理模块
│   ├── wifi_manager.h     # Wi-Fi管理模块头文件
│   ├── tcp_server.c       # TCP服务器实现
│   ├── tcp_server.h       # TCP服务器头文件
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
   - `WIFI_AP_SSID`：WiFi热点名称（默认为"sample_project_ap"）
   - `WIFI_AP_PASSWORD`：WiFi密码（默认为"12345678"）

2. **TCP服务器配置**：
   - `TCP_SERVER_PORT`：TCP服务器监听端口（默认为8888）
   - `TCP_BUFFER_SIZE`：TCP数据缓冲区大小（默认为1024字节）
   - `TCP_MAX_CLIENTS`：最大客户端连接数（默认为5）

3. **统一日志接口**：
   - `TCP_TAG`：日志标签（"tcp_server"）
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

    //连接到AP
    bk_err_t bk_wifi_sta_start(void);

    //获取STA连接状态、IP地址等信息
    bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status);

    //断开连接
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

    //绑定套接字到指定端口
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

    //开始监听
    int listen(int sockfd, int backlog);

    //接受客户端连接
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

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
        // 初始化Wi-Fi STA模式并连接到AP
        ret = wifi_sta_connect(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        if (ret != BK_OK)
        {
            TCP_LOGE("Failed to connect WiFi: %d\n", ret);
            break;
        }

        // 创建TCP服务器线程
        ret = tcp_server_create_thread();
        if (ret != BK_OK)
        {
            TCP_LOGE("create tcp server thread failed: %d\n", ret);
            break;
        }
        TCP_LOGI("create tcp server thread success\n");
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

        // 启动WiFi STA模式并连接
        ret = bk_wifi_sta_start();
        if (ret != BK_OK)
        {
            TCP_LOGE("Failed to start WiFi STA: %d\n", ret);
            break;
        }

        TCP_LOGI("WiFi STA connecting to SSID: %s\n", sta_config.ssid);
    } while (0);
    
    return ret;
}
```

### TCP服务器线程创建（tcp_server.c）
``` C
bk_err_t tcp_server_create_thread(void)
{
    beken_thread_t tcp_thread;
    bk_err_t ret = BK_OK;
    
    do {
        // 创建TCP服务器线程
        ret = rtos_create_thread(&tcp_thread, BEKEN_APPLICATION_PRIORITY, "tcp_server", 
                               tcp_server_thread, 4096, NULL);
        if (ret != BK_OK)
        {
            TCP_LOGE("create tcp server thread failed: %d\n", ret);
            break;
        }
        TCP_LOGI("create tcp server thread success\n");
    } while (0);

    return ret;
}
```

### TCP服务器实现（tcp_server.c）
``` C
void tcp_server_thread(void *arg)
{
    int server_sockfd = -1;
    int client_sockfd = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int ret;
    int opt = 1;

    TCP_LOGI("TCP server thread started\n");

    // 等待网络连接稳定
    while(1)
    {
        if(wifi_sta_is_link_success()) break;
        rtos_delay_milliseconds(100);
    }

    // 创建TCP套接字
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0)
    {
        TCP_LOGE("socket create failed: %d\n", server_sockfd);
        goto exit;
    }
    TCP_LOGI("socket create success\n");

    // 设置套接字选项，允许地址重用
    ret = setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0)
    {
        TCP_LOGE("setsockopt failed: %d\n", ret);
        goto exit;
    }

    // 设置服务器地址
    os_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有网络接口
    server_addr.sin_port = htons(TCP_SERVER_PORT);

    // 绑定套接字到指定端口
    ret = bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        TCP_LOGE("bind failed: %d\n", ret);
        goto exit;
    }
    TCP_LOGI("bind to port %d success\n", TCP_SERVER_PORT);

    // 开始监听
    ret = listen(server_sockfd, TCP_MAX_CLIENTS);
    if (ret < 0)
    {
        TCP_LOGE("listen failed: %d\n", ret);
        goto exit;
    }
    TCP_LOGI("listening on port %d, max clients: %d\n", TCP_SERVER_PORT, TCP_MAX_CLIENTS);

    // 循环接受客户端连接
    while (1)
    {
        // 接受客户端连接
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sockfd < 0)
        {
            TCP_LOGE("accept failed: %d\n", client_sockfd);
            continue;
        }

        TCP_LOGI("accepted client connection\n");

        // 为每个客户端创建处理线程
        beken_thread_t client_thread;
        ret = rtos_create_thread(&client_thread, BEKEN_APPLICATION_PRIORITY, "tcp_client", 
                               tcp_client_handler, 4096, (void *)client_sockfd);
        if (ret != BK_OK)
        {
            TCP_LOGE("create client handler thread failed: %d\n", ret);
            close(client_sockfd);
            continue;
        }
    }

exit:
    if (server_sockfd >= 0)
    {
        close(server_sockfd);
        TCP_LOGI("server socket closed\n");
    }
    rtos_delete_thread(NULL); // 删除当前线程
}
```

### 客户端处理线程（tcp_server.c）
``` C
static void tcp_client_handler(void *arg)
{
    int client_sockfd = (int)arg;
    char buffer[TCP_BUFFER_SIZE];
    int ret;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 获取客户端地址信息
    getpeername(client_sockfd, (struct sockaddr *)&client_addr, &addr_len);
    TCP_LOGI("client connected from %s:%d\n", 
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // 循环接收并回复数据
    while (1)
    {
        // 接收客户端数据
        ret = recv(client_sockfd, buffer, TCP_BUFFER_SIZE - 1, 0);
        if (ret <= 0)
        {
            if (ret == 0)
            {
                TCP_LOGI("client disconnected\n");
            }
            else
            {
                TCP_LOGE("recv failed: %d\n", ret);
            }
            break;
        }

        // 添加字符串结束符
        buffer[ret] = '\0';
        TCP_LOGI("received from client: %s\n", buffer);

        // 直接回复收到的数据
        ret = send(client_sockfd, buffer, ret, 0);
        if (ret < 0)
        {
            TCP_LOGE("send reply failed: %d\n", ret);
            break;
        }
        TCP_LOGI("send reply success\n");
    }

    // 关闭客户端套接字
    close(client_sockfd);
    TCP_LOGI("client socket closed\n");
    rtos_delete_thread(NULL); // 删除当前线程
}
```

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/net_protocol/tcp_server
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录build文件夹里的all-app.bin;
- 板子上电运行后，板子会自动连接到"sample_project_ap"热点，密码为"12345678"；
- 板子会创建TCP服务器，监听8888端口，最多支持5个客户端同时连接；
- 使用TCP客户端工具（如sscom32、telnet、netcat等）连接到板子的IP地址和8888端口；
- 发送数据后，服务器会自动回复相同的数据。

以下是上电运行后的主要log输出：

```
[2025-12-30 14:52:53.951]  ap1:tcp_serv:I(6494):STA got ip:192.168.50.202   //连接wifi成功，并获取ip
[2025-12-30 14:52:53.951]  ap0:tcp_serv:I(6529):socket create success
[2025-12-30 14:52:53.951]  ap0:tcp_serv:I(6530):bind to port 8888 success
[2025-12-30 14:52:54.060]  ap0:tcp_serv:I(6530):listening on port 8888, max clients: 5   //tcp服务器监听8888端口，最多支持5个客户端同时连接
[2025-12-30 14:53:21.671]  ap1:tcp_serv:I(34160):accepted client connection
[2025-12-30 14:53:21.671]  ap1:tcp_serv:I(34161):client connected from 192.168.50.207:60587  //客户端连接，客户端ip为192.168.50.207，端口为60587
[2025-12-30 14:53:45.399]  ap1:tcp_serv:I(57872):received from client: hello, this is from beken tcp client   //收到tcp 客户端数据
```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
