#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdio.h>

#include "bk_posix.h"

#define TAG "file_sys"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define MOUNT_PATH "/"
static bool sdcard_mounted = false;

/**
 * @brief 挂载SD卡FATFS文件系统
 * @return 0 成功；负值 错误码
 */
int sdcard_fatfs_mount()
{
    int ret = BK_OK;
    if(sdcard_mounted)
    {
        LOGI("sdcard already mounted\n");
        return ret;
    }

    const char *fs_name = "fatfs";
    struct bk_fatfs_partition partition;
    partition.part_type = FATFS_DEVICE;
    partition.part_dev.device_name = FATFS_DEV_SDCARD;
    partition.mount_path = MOUNT_PATH;

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
    if (ret != BK_OK)
    {
        LOGE("sdcard mount failed:%d\r\n", ret);
    }
    else
    {
        sdcard_mounted = true;
        LOGI("sdcard mount OK!\r\n");
    }
    return ret;
}

/**
 * @brief 卸载SD卡FATFS文件系统
 * @return 0 成功；负值 错误码
 */
int sdcard_fatfs_umount()
{
    int ret = BK_OK;
    if(!sdcard_mounted)
    {
        LOGI("sdcard not mounted\n");
        return BK_FAIL;
    }

    ret = umount(MOUNT_PATH);
    if (BK_OK != ret)
    {
        LOGD("[%s][%d] unmount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
    }
    else
    {
        LOGD("[%s][%d] unmount success\r\n", __FUNCTION__, __LINE__);
        sdcard_mounted = false;
    }
    return ret;
}
