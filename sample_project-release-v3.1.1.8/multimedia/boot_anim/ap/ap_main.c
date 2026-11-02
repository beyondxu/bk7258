#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include "bk_posix.h"
#include "driver/flash_partition.h"
#include "components/ate.h"
#include "components/media_types.h"

#include "common.h"
#include "dev_lcd.h"
#include "boot_animation.h"

#define TAG "main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

//挂载文件系统
static bk_err_t _mount_fs(char *mount_path)
{
    struct bk_little_fs_partition partition;
    char *fs_name = NULL;
    bk_err_t ret = BK_FAIL;
    bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
    uint32_t flash_start = fs_partition->partition_start_addr;
    uint32_t flash_size = fs_partition->partition_length;

    fs_name = "littlefs";
    partition.part_type = LFS_FLASH;
    partition.part_flash.start_addr = flash_start;
    partition.part_flash.size = flash_size;
    partition.mount_path = mount_path;

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

    LOGI("[%s][%d] mount ret:%d, start_addr:%p, flash_size:%p\r\n", __FUNCTION__, __LINE__, ret, flash_start, flash_size);
    
    return ret;
}

int main(void)
{
    bk_init();
    media_service_init();

    if(1/*0 == ate_is_enabled()*/)   //非ATE模式才启动应用代码
    {
        //文件系统初始化
        if(BK_OK != _mount_fs(VFS_MOUNT_PATH))
        {
            LOGE("[%s][%d] fs mount fail\r\n", __FUNCTION__, __LINE__);
            TEST_FAIL();
            return BK_FAIL;
        }

        dev_lcd_open();                    //打开lcd
        dev_lcd_backlight_ctrl(100);       //开背光

        boot_animation();                  //启动动画
        TEST_PASS();
    }

    return 0;
}
