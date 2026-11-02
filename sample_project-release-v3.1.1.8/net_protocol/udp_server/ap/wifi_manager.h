/*
 * wifi_manager.h
 * WiFi管理模块头文件
 * 提供WiFi STA模式的初始化、连接和事件处理功能
 */

#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <os/os.h>

/**
 * @brief 初始化Wi-Fi STA模式并尝试连接到指定的Wi-Fi网络
 * @param ssid WiFi热点名称
 * @param password WiFi密码
 * @return 操作结果，成功返回BK_OK，失败返回错误码
 */
int wifi_sta_connect(const char *ssid, const char *password);

/**
 * @brief wifi是否连接成功
 * @param 无
 * @return true 连接成功  false 正在连接
 */
bool wifi_sta_is_link_success(void);

#endif /* __WIFI_MANAGER_H__ */
