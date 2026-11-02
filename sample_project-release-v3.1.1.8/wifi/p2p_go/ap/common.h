/*
 * common.h
 * 公共头文件
 * 包含项目通用的定义和接口
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <components/log.h>

// 统一日志接口
#define P2P_TAG "p2p_gc"
#define P2P_LOGI(...) BK_LOGI(P2P_TAG, ##__VA_ARGS__)
#define P2P_LOGE(...) BK_LOGE(P2P_TAG, ##__VA_ARGS__)
#define P2P_LOGD(...) BK_LOGD(P2P_TAG, ##__VA_ARGS__)
#define P2P_LOGV(...) BK_LOGV(P2P_TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(P2P_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(P2P_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")


#endif /* __COMMON_H__ */
