#include <os/os.h>
#include <os/str.h>
#include <components/system.h>
#include <stdlib.h>
#include "ftp_session.h"
#include "ftp_session_cmd.h"
#include "lwip/errno.h"

#define DBG_TAG "ftp_session"
#define LOG_I(fmt, ...) bk_printf("[%s][INFO] " fmt, DBG_TAG, ##__VA_ARGS__)
#define LOG_W(fmt, ...) bk_printf("[%s][WARN] " fmt, DBG_TAG, ##__VA_ARGS__)
#define LOG_E(fmt, ...) bk_printf("[%s][ERROR] " fmt, DBG_TAG, ##__VA_ARGS__)

#ifndef FTP_MAX_SESSION_NUM
#define FTP_MAX_SESSION_NUM             10
#endif

#ifndef FTP_SESSION_USERNAME
#define FTP_SESSION_USERNAME            "admin"
#endif

#ifndef FTP_SESSION_PASSWORD
#define FTP_SESSION_PASSWORD            "admin"
#endif

#ifndef FTP_SESSION_WELCOME_MSG
#define FTP_SESSION_WELCOME_MSG         "220 -= welcome on Beken FTP server =-\r\n"
#endif

#ifndef FTP_SESSION_TIMEOUT
#define FTP_SESSION_TIMEOUT             300
#endif

static int ftp_max_session_num = FTP_MAX_SESSION_NUM;
static char ftp_session_username[64] = FTP_SESSION_USERNAME;
static char ftp_session_password[64] = FTP_SESSION_PASSWORD;
static char ftp_session_welcome_msg[100] = FTP_SESSION_WELCOME_MSG;
static struct slist_node session_header = {NULL};

int ftp_get_max_session_num(void)
{
    return ftp_max_session_num;
}

int ftp_set_max_session_num(int num)
{
    if(num <= 0)
        return -1;

    ftp_max_session_num = num;
    return BK_OK;
}

const char *ftp_get_session_username(void)
{
    return ftp_session_username;
}

int ftp_set_session_username(const char *username)
{
    if(username == NULL)
        return -1;

    strncpy(ftp_session_username, username, sizeof(ftp_session_username) - 1);
    ftp_session_username[sizeof(ftp_session_username) - 1] = '\0';
    return BK_OK;
}

const char *ftp_get_session_password(void)
{
    return ftp_session_password;
}

int ftp_set_session_password(const char *password)
{
    if(password == NULL)
        return -1;

    strncpy(ftp_session_password, password, sizeof(ftp_session_password) - 1);
    ftp_session_password[sizeof(ftp_session_password) - 1] = '\0';
    return BK_OK;
}

const char *ftp_get_session_welcome_msg(void)
{
    return ftp_session_welcome_msg;
}

int ftp_set_session_welcome_msg(const char *welcome_msg)
{
    if(welcome_msg == NULL)
        return -1;

    strncpy(ftp_session_welcome_msg, welcome_msg, sizeof(ftp_session_welcome_msg) - 1);
    ftp_session_welcome_msg[sizeof(ftp_session_welcome_msg) - 1] = '\0';
    return BK_OK;
}

static int ftp_session_get_num(void)
{
    int num = 0;
    struct slist_node *node;

    node = session_header.next;
    while(node != NULL)
    {
        num++;
        node = node->next;
    }

    return num;
}

static int ftp_session_delete(struct ftp_session * session)
{
    struct slist_node *prev = &session_header;
    struct slist_node *node = session_header.next;

    while(node != NULL)
    {
        if(node == &(session->slist))
        {
            prev->next = node->next;
            break;
        }
        prev = node;
        node = node->next;
    }

    close(session->fd);
    if(session->port_pasv_fd >= 0)
        close(session->port_pasv_fd);
    free(session);

    return BK_OK;
}

static int ftp_session_read(struct ftp_session *session, uint8_t *buf, int bufsz, int timeout)
{
    int bytes = 0;
    int rc = 0;

    if(bufsz <= 0)
        return bufsz;

    while(bytes < bufsz)
    {
        rc = recv(session->fd, &buf[bytes], (size_t)(bufsz - bytes), MSG_DONTWAIT);
        if(rc < 0)
        {
            // If it's just a temporary error (no data available), return what we have
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if(bytes > 0)
                    return bytes;
                return -1;
            }
            // For other errors, return -1
            return -1;
        }
        else if(rc == 0)
        {
            // Connection closed by client, return what we have if any
            return bytes > 0 ? bytes : -1;
        }

        bytes += rc;
        if(bytes >= bufsz)
            break;

        if(timeout > 0)
        {
            fd_set readset, exceptset;
            struct timeval interval;

            interval.tv_sec = timeout / 1000;
            interval.tv_usec = (timeout % 1000) * 1000;

            FD_ZERO(&readset);
            FD_ZERO(&exceptset);
            FD_SET(session->fd, &readset);
            FD_SET(session->fd, &exceptset);

            rc = select(session->fd + 1, &readset, NULL, &exceptset, &interval);
            if(rc < 0)
                return -1;
            if(rc == 0)
                break;
            if(FD_ISSET(session->fd, &exceptset))
                return -1;
        }
        else
            break;
    }

    return bytes;
}

static int ftp_session_process(struct ftp_session * session, char *cmd_buf)
{
    int result = BK_OK;

    /* remove \r\n */
    char *ptr = cmd_buf;
    while (*ptr)
    {
        if ((*ptr == '\r') || (*ptr == '\n'))
            *ptr = 0;
        ptr ++;
    }

    char *cmd = cmd_buf;
    char *cmd_param = strchr(cmd, ' ');
    if(cmd_param)
    {
        *cmd_param = '\0';
        cmd_param++;
    }

    LOG_I("ftp_session_process: %s %s\n", cmd, cmd_param);
    switch(session->state)
    {
        case FTP_SESSION_STATE_USER:
        {
            if(strstr(cmd, "USER") != cmd)
            {
                char *reply = "502 Not Implemented.\r\n";
                send(session->fd, reply, strlen(reply), 0);
                break;
            }

            if(strcmp(cmd_param, "anonymous") == 0)
            {
                session->is_anonymous = 1;
                char *reply = "331 anonymous login OK send e-mail address for password.\r\n";
                send(session->fd, reply, strlen(reply), 0);
                session->state = FTP_SESSION_STATE_PASSWD;
                break;
            }

            if(strcmp(cmd_param, ftp_session_username) == 0)
            {
                session->is_anonymous = 0;
                char *reply = "331 Password required.\r\n";
                send(session->fd, reply, strlen(reply), 0);
                session->state = FTP_SESSION_STATE_PASSWD;
                break;
            }

            char *reply = "530 Login incorrect. Bye.\r\n";
            send(session->fd, reply, strlen(reply), 0);
            result = -1;
        }
        break;

        case FTP_SESSION_STATE_PASSWD:
        {
            if(strstr(cmd, "PASS") != cmd)
            {
                char *reply = "502 Not Implemented.\r\n";
                send(session->fd, reply, strlen(reply), 0);
                break;
            }

            if(session->is_anonymous || (strcmp(cmd_param, ftp_session_password) == 0))
            {
                char *reply = "230 User logged in\r\n";
                send(session->fd, reply, strlen(reply), 0);
                memset(session->currentdir, 0, sizeof(session->currentdir));
                session->currentdir[0] = '/';
                session->state = FTP_SESSION_STATE_PROCESS;
                break;
            }

            char *reply = "530 Login incorrect. Bye.\r\n";
            send(session->fd, reply, strlen(reply), 0);
            result = -1;
        }
        break;

        case FTP_SESSION_STATE_PROCESS:
        {
            int rc = ftp_session_cmd_process(session, cmd, cmd_param);
            if(rc != BK_OK)
            {
                result = -1;
                break;
            }
        }
        break;

        default:
            result = -1;
            break;
    }

    session->tick_timeout = rtos_get_time() + FTP_SESSION_TIMEOUT;

    return result;
}

static void ftp_client_entry(void *parameter)
{
    struct ftp_session *session = parameter;
    int option = 1;
    int rc = setsockopt(session->fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&option, sizeof(int));
    if(rc < 0)
        goto _exit;

    // 使用lwip_fcntl设置非阻塞模式
    int flags = lwip_fcntl(session->fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    lwip_fcntl(session->fd, F_SETFL, flags);

    session->port_pasv_fd = -1;
    session->is_anonymous = 0;
    session->offset = 0;
    session->state = FTP_SESSION_STATE_USER;
    session->tick_timeout = rtos_get_time() + FTP_SESSION_TIMEOUT;

    char cmd_buf[1024];

    // select使用
    fd_set readset, exceptset;
    // select超时时间
    struct timeval select_timeout;
    select_timeout.tv_sec = 1;
    select_timeout.tv_usec = 0;

    send(session->fd, ftp_session_welcome_msg, strlen(ftp_session_welcome_msg), 0);

    while(1)
    {
        if(session->force_quit)
            break;

        FD_ZERO(&readset);
        FD_ZERO(&exceptset);

        FD_SET(session->fd, &readset);
        FD_SET(session->fd, &exceptset);

        rc = select(session->fd + 1, &readset, NULL, &exceptset, &select_timeout);
        if(rc < 0)
            break;
        if(rc > 0)
        {
            if(FD_ISSET(session->fd, &exceptset))
                break;
            if(FD_ISSET(session->fd, &readset))
            {
                int cmd_len = ftp_session_read(session, (uint8_t *)cmd_buf, sizeof(cmd_buf) - 1, 30);
                if(cmd_len <= 0)
                    break;
                cmd_buf[cmd_len] = '\0';

                if(ftp_session_process(session, cmd_buf) != BK_OK)
                {
                    LOG_E("ftp_session_process: error.\n");
                    break;
                }
            }
        }

        if(rtos_get_time() > session->tick_timeout)
        {
            LOG_E("ftp_client_entry: timeout:%d, tick_timeout:%d.\n", rtos_get_time(), session->tick_timeout);
            break;
        }
    }

_exit:
    LOG_I("ftp_client_entry: exit.\n");
    ftp_session_delete(session);
    rtos_delete_thread(NULL);
}

int ftp_session_create(int fd, struct sockaddr_in *addr, socklen_t addr_len)
{
    if(fd < 0)
        return -1;

    if(ftp_session_get_num() >= ftp_max_session_num)
        return -1;

    struct ftp_session *session = (struct ftp_session *)malloc(sizeof(struct ftp_session));
    if(session == NULL)
        return -1;
    memset(session, 0, sizeof(struct ftp_session));
    session->fd = fd;
    memcpy(&(session->remote), addr, addr_len);
    session->slist.next = NULL;

    // 添加到会话列表
    session->slist.next = session_header.next;
    session_header.next = &(session->slist);

    beken_thread_t tid;
    bk_err_t result = rtos_create_thread(&tid, 5, "ftp_session",
                                         (beken_thread_function_t)ftp_client_entry, 4096, session);
    if(result != BK_OK)
    {
        LOG_E("create session thread failed.");
        free(session);
        return -1;
    }

    LOG_I("new session created.");
    return BK_OK;
}

int ftp_session_force_quit(void)
{
    struct slist_node *node = session_header.next;
    struct ftp_session *session;

    while(node != NULL)
    {
        session = (struct ftp_session *)((char *)node - offsetof(struct ftp_session, slist));
        session->force_quit = 1;
        node = node->next;
    }

    return BK_OK;
}