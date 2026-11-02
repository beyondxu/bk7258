# UDP服务器示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 Wi-Fi 组件和 UDP 协议，演示了一个简单的 UDP 服务器实现。它具体展示了如何：

1. 配置 Wi-Fi STA 参数，连接到 AP
2. 使用 os.h rtos_create_thread 接口创建线程
3. 使用 lwIP 网络栈实现 UDP 服务器功能
4. 监听指定端口，循环接收和回复数据

## 项目结构

```
udp_server/
├── ap/
│   ├── ap_main.c          # 主程序入口
│   ├── wifi_manager.c     # Wi-Fi管理模块
│   ├── wifi_manager.h     # Wi-Fi管理模块头文件
│   ├── udp_server.c       # UDP服务器实现
│   ├── udp_server.h       # UDP服务器头文件
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

2. **UDP服务器配置**：
   - `UDP_SERVER_PORT`：UDP服务器监听端口（默认为8888）
   - `UDP_BUFFER_SIZE`：UDP数据缓冲区大小（默认为1024字节）

3. **统一日志接口**：
   - `UDP_TAG`：日志标签（"udp_server"）
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

### UDP接口
更多接口请查看lwip/sockets.h、lwip/inet.h。

``` C
    //创建套接字
    int socket(int domain, int type, int protocol);

    //绑定套接字到指定端口
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

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
        // 初始化Wi-Fi STA模式并连接到AP
        ret = wifi_sta_connect(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to connect WiFi: %d\n", ret);
            break;
        }

        // 创建UDP服务器线程
        ret = udp_server_create_thread();
        if (ret != BK_OK)
        {
            UDP_LOGE("create udp server thread failed: %d\n", ret);
            break;
        }
        UDP_LOGI("create udp server thread success\n");
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

        // 启动WiFi STA模式并连接
        ret = bk_wifi_sta_start();
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to start WiFi STA: %d\n", ret);
            break;
        }

        UDP_LOGI("WiFi STA connecting to SSID: %s\n", sta_config.ssid);
    } while (0);
    
    return ret;
}
```

### UDP服务器线程创建（udp_server.c）
``` C
bk_err_t udp_server_create_thread(void)
{
    beken_thread_t udp_thread;
    bk_err_t ret = BK_OK;
    
    do {
        // 创建UDP服务器线程
        ret = rtos_create_thread(&udp_thread, BEKEN_APPLICATION_PRIORITY, "udp_server", 
                               udp_server_thread, 4096, NULL);
        if (ret != BK_OK)
        {
            UDP_LOGE("create udp server thread failed: %d\n", ret);
            break;
        }
        UDP_LOGI("create udp server thread success\n");
    } while (0);

    return ret;
}
```

### UDP服务器实现（udp_server.c）
``` C
void udp_server_thread(void *arg)
{
    int server_sockfd = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int ret;
    char buffer[UDP_BUFFER_SIZE];

    UDP_LOGI("UDP server thread started\n");

    // 等待网络连接稳定
    while(1)
    {
        if(wifi_sta_is_link_success()) break;
        rtos_delay_milliseconds(100);
    }

    // 创建UDP套接字
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0)
    {
        UDP_LOGE("socket create failed: %d\n", server_sockfd);
        goto exit;
    }
    UDP_LOGI("socket create success\n");

    // 设置服务器地址
    os_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有网络接口
    server_addr.sin_port = htons(UDP_SERVER_PORT);

    // 绑定套接字到指定端口
    ret = bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        UDP_LOGE("bind failed: %d\n", ret);
        goto exit;
    }
    UDP_LOGI("bind to port %d success\n", UDP_SERVER_PORT);

    // 循环接收和回复数据
    while (1)
    {
        // 接收客户端数据
        ret = recvfrom(server_sockfd, buffer, UDP_BUFFER_SIZE - 1, 0, 
                      (struct sockaddr *)&client_addr, &addr_len);
        if (ret < 0)
        {
            UDP_LOGE("recvfrom failed: %d\n", ret);
            continue;
        }

        // 添加字符串结束符
        buffer[ret] = '\0';
        UDP_LOGI("received from %s:%d: %s\n", 
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);

        // 直接回复收到的数据
        ret = sendto(server_sockfd, buffer, ret, 0, 
                    (struct sockaddr *)&client_addr, addr_len);
        if (ret < 0)
        {
            UDP_LOGE("sendto failed: %d\n", ret);
            continue;
        }
        UDP_LOGI("send reply to %s:%d success\n", 
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

exit:
    if (server_sockfd >= 0)
    {
        close(server_sockfd);
        UDP_LOGI("server socket closed\n");
    }
    rtos_delete_thread(NULL); // 删除当前线程
}
```

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/net_protocol/udp_server
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录build文件夹里的all-app.bin;
- 板子上电运行后，板子会自动连接到"sample_project_ap"热点，密码为"12345678"；
- 板子会创建UDP服务器，监听8888端口；
- 使用UDP客户端工具（如sscom32、netcat等）向板子的IP地址和8888端口发送数据；
- 发送数据后，服务器会自动回复相同的数据。

以下是上电运行后的主要log输出：

```
[2025-12-30 16:00:16.869]  ap0:cli:W(1056):BK STA connected E0_xiao
[2025-12-30 16:00:16.869]  ap0:udp_serv:I(1056):STA connected to E0_xiao
[2025-12-30 16:00:16.869]  ap0:cli:W(1056):BK STA got ip
[2025-12-30 16:00:16.870]  ap0:udp_serv:I(1056):STA got ip:192.168.50.202   //连接wifi，并获取ip
[2025-12-30 16:00:16.870]  ap0:udp_serv:I(1122):socket create success
[2025-12-30 16:00:16.870]  ap0:udp_serv:I(1122):bind to port 8888 success  //侦听8888端口 udp数据
[2025-12-30 16:00:36.538]  ap0:udp_serv:I(20816):received from 192.168.50.207:8888: hello, this is from pc's udp client //收到udp 客户端发送的消息
[2025-12-30 16:00:36.538]  ap0:udp_serv:I(20819):send reply to 192.168.50.207:8888 success  //收到数据回复给客户端
[2025-12-30 16:00:44.416]  ap0:udp_serv:I(28601):received from 192.168.50.207:8888: hello, this is from pc's udp client

```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。