#include <os/os.h>
#include <os/str.h>
#include <components/system.h>

// 我们使用lwip提供的socket API
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "ftp_session.h"

#define DBG_TAG "ftp"
#define LOG_I(fmt, ...) bk_printf("[%s][INFO] " fmt, DBG_TAG, ##__VA_ARGS__)
#define LOG_W(fmt, ...) bk_printf("[%s][WARN] " fmt, DBG_TAG, ##__VA_ARGS__)

#ifndef FTP_DEFAULT_PORT
#define FTP_DEFAULT_PORT        21
#endif

static int ftp_port = FTP_DEFAULT_PORT;
static uint8_t force_restart = 0;

int ftp_force_restart(void)
{
    force_restart = 1;
    return BK_OK;
}

int ftp_get_port(void)
{
    return ftp_port;
}

int ftp_set_port(int port)
{
    if((port <= 0) || (port > 65535))
        return -1;
    ftp_port = port;
    return BK_OK;
}

static void ftp_entry(void *parameter)
{
    int server_fd = -1;
    int enable = 1;
    struct sockaddr_in addr;
    socklen_t addrlen;

    // select使用
    fd_set readset, exceptset;
    // select超时时间
    struct timeval select_timeout;
    select_timeout.tv_sec = 1;
    select_timeout.tv_usec = 0;

_ftp_start:
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server_fd < 0)
        goto _ftp_restart;

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&enable, sizeof(enable)) < 0)
        goto _ftp_restart;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ftp_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        goto _ftp_restart;

    if(listen(server_fd, 1) < 0)
        goto _ftp_restart;

    // 使用lwip_fcntl设置非阻塞模式
    int flags = lwip_fcntl(server_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    lwip_fcntl(server_fd, F_SETFL, flags);

    LOG_I("service launched success.\n");
    while(1)
    {
        if(force_restart)
        {
            force_restart = 0;
            ftp_session_force_quit();
            break;
        }

        FD_ZERO(&readset);
        FD_ZERO(&exceptset);

        FD_SET(server_fd, &readset);
        FD_SET(server_fd, &exceptset);

        int rc = select(server_fd + 1, &readset, NULL, &exceptset, &select_timeout);
        if(rc < 0)
            break;
        if(rc > 0)
        {
            if (FD_ISSET(server_fd, &exceptset))
                break;
            if (FD_ISSET(server_fd, &readset))
            {
                addrlen = sizeof(struct sockaddr_in);
                int client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
                if(client_fd < 0)
                    break;
                if(ftp_session_create(client_fd, &addr, addrlen) != BK_OK)
                    close(client_fd);
            }
        }
    }

_ftp_restart:
    LOG_W("service go wrong, now wait restarting...\n");
    if(server_fd >= 0)
    {
        close(server_fd);
        server_fd = -1;
    }

    rtos_delay_milliseconds(1000);
    goto _ftp_start;
}

int ftp_init(uint32_t stack_size, uint8_t priority)
{
    beken_thread_t tid;
    int err = rtos_create_thread(&tid, priority, "ftp",
                                 (beken_thread_function_t)ftp_entry, stack_size, NULL);
    if (err != kNoErr)
        return -1;

    return BK_OK;
}