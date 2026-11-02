#ifndef __FTP_SESSION_H
#define __FTP_SESSION_H
#include <os/os.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <os/mem.h>
#include <string.h>

// 定义链表节点结构
struct slist_node
{
    struct slist_node *next;
};

enum ftp_session_state
{
    FTP_SESSION_STATE_USER = 0,
    FTP_SESSION_STATE_PASSWD,
    FTP_SESSION_STATE_PROCESS
};

struct ftp_session
{
    int fd;
    int port_pasv_fd;
    int is_anonymous;
    int offset;
    struct sockaddr_in remote;
    enum ftp_session_state state;
    char currentdir[256];
    uint8_t force_quit;
    uint32_t tick_timeout;
    struct slist_node slist;
};


int ftp_session_create(int fd, struct sockaddr_in *addr, socklen_t addr_len);
int ftp_session_force_quit(void);

#endif
