#include <stdio.h>
#include <string.h>

#include <os/os.h>
#include <os/str.h>
#include <common/bk_include.h>
#include "bk_kws_asr.h"

#include "app_asr_intf.h"
#include "bk_app_asr.h"

#define TAG  "kws"

#define LOGE(format, ...) BK_LOGE(TAG, "[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGI(format, ...) BK_LOGI(TAG, "[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) BK_LOGD(TAG, "[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(format, ...) BK_LOGW(TAG, "[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

extern int bkkws_authentication_init(const char* auth_id, const char* auth_key, uint32_t base);

static bool s_kws_inited = false;
static asr_auth_t s_auth = {0};

static int _kws_asr_open(void)
{
    bk_err_t ret = BK_OK;

    if (s_kws_inited) 
    {
        LOGW("bk kws already inited");
        return BK_OK;
    }

    ret = bk_app_asr_get_auth(&s_auth);
    if (ret != BK_OK) 
    {
        LOGW("kws auth info not found\r\n");
    }

    LOGI("auth_id: %s, auth_key: %s\r\n", s_auth.id, s_auth.key);

    ret = bkkws_authentication_init(s_auth.id, s_auth.key, CONFIG_PSRAM_MEM_SLAB_KWS_ADDR);
    if (ret != BK_OK) 
    {
        LOGE("bk kws auth fail, ret: %d\n", ret);
    } else {
        LOGI("bk kws auth success\r\n");
    }
    
    ret = bk_kws_init();
    if (ret != BK_OK) 
    {
        LOGE("bk kws init fail, ret: %d\n", ret);
        return BK_FAIL;
    }

    s_kws_inited = true;

    return BK_OK;
}

static void _kws_asr_close(void)
{
    return;
}

static int _kws_asr_process(void *data, uint32_t len, void *p1, void *p2)
{
    if (!s_kws_inited) 
    {
        return -1;
    }

    return bk_kws_recog((short *)data, len, p1, p2);
}

const app_asr_intf_t *kws_asr_instance(void)
{
    static const app_asr_intf_t kws_asr_intf = 
    {
        .name    = "kws",
        .open    = _kws_asr_open,
        .close   = _kws_asr_close,
        .process = _kws_asr_process,
    };
    
    return &kws_asr_intf;
}
