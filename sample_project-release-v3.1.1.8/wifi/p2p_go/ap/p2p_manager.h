#ifndef __P2P_MANAGER_H__
#define __P2P_MANAGER_H__

#include <os/os.h>

/**
 * @brief 启动P2P GC（Group Client）模式并开始设备发现
 * @param ssid P2P组SSID名称
 * @return BK_OK成功，其他错误码表示失败
 */
int app_p2p_gc_connect(const char *ssid);

/**
 * @brief 启动P2P GO（Group Owner）模式并开始设备发现
 * @param ssid P2P组SSID名称
 * @return BK_OK成功，其他错误码表示失败
 */
int app_p2p_go_start(const char *ssid);

/**
 * @brief 断开P2P GO（Group Owner）或GC（Group Client）连接
 * @return BK_OK成功，其他错误码表示失败
 */
int app_p2p_disconnect(void);

#endif
