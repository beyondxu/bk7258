#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include <bk_private/bk_wifi.h>
#include <lwip/inet.h>
#include "common.h"
#include "p2p_manager.h"

#define P2P_GO_NAME     "P2P_GO_Device"

int main(void)
{
    int ret = 0;

    bk_init();
    
    ret = app_p2p_go_start(P2P_GO_NAME);
    if (ret != BK_OK)
    {
        P2P_LOGE("Failed to start P2P GO: %d\n", ret);
        TEST_FAIL();
        return ret;
    }
    P2P_LOGI("P2P GO started, waiting for connection...\n");

    TEST_PASS();
    return ret;
}
