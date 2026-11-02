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
#include "frame_buffer.h"
#include "driver/pwr_clk.h"
#include "jpeg_hw_decode.h"
#include "frame_buffer_app.h"

#define TAG "VFS"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CONFIG_INIT_EMPTY_FS     0
#define VFS_MOUNT_PATH           "/littlefs"

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

jpeg_hw_decode_config_t g_jpeg_hw_decode_config = {
            .jpeg_decode_handle = NULL,                     //由接口内部申请解码handle
            .jpeg_frame_malloc = jpeg_frame_malloc,        //jpeg frame申请接口
            .jpeg_frame_free = jpeg_frame_free,            //jpeg frame释放接口
            .decode_frame_malloc = decode_frame_malloc,    //解码 frame申请接口
            .decode_frame_free = decode_frame_free,        //解码 frame释放接口

            .use_dma2d = false,
            .byte_swap = false,
            .output_width = 0,
            .output_height = 0,
};

void _decode_handle(beken_thread_arg_t arg)
{
    frame_buffer_t *decode_frame = NULL;
    bk_err_t ret = BK_FAIL;

    rtos_delay_milliseconds(500); //等待系统初始化完成
    
    ret = jpeg_hw_decode_from_file_to_yuv422(&g_jpeg_hw_decode_config, VFS_MOUNT_PATH"/img/yuv422/example_jpg422.jpg", &decode_frame);
    if(BK_OK == ret)
    {
        TEST_PASS();

        //写入ota分区偏移100K，使用bkfil.exe读出，并用yuvplayer查看解码内容
        #if 0
        uint32 offset = 100*1024;
        bk_flash_partition_erase(BK_PARTITION_OTA, offset, 1024*1024);
        bk_flash_partition_write(BK_PARTITION_OTA, decode_frame->frame, offset, decode_frame->length);
        #endif
        
        if (decode_frame) 
        {
            decode_frame_free(decode_frame);   //释放解码内存
        }
    }
    else
    {
        TEST_FAIL();
    }
}

int main(void)
{
    bk_init();
    media_service_init();

    #if !USE_PSRAM_MALLOC  //使用frame_buffer里的malloc接口，需要对psram进行上电投票
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_APP, PM_POWER_MODULE_STATE_ON);
    #endif

    //文件系统初始化
    if(BK_OK != _mount_fs(VFS_MOUNT_PATH))
    {
        LOGE("[%s][%d] fs mount fail\r\n", __FUNCTION__, __LINE__);
        TEST_FAIL();
        return BK_FAIL;
    }

    int ret;
    beken_thread_t jpeg_thread;
    
    /* create a thread on core 1 */
    ret = rtos_create_thread(&jpeg_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "jpeg_thread",
                             (beken_thread_function_t)_decode_handle ,
                             2048,
                             0);
    if (ret != kNoErr) 
    {
        LOGE("[%s][%d] Failed to create task: %d\r\n", __FUNCTION__, __LINE__);
    }
    
    return 0;
}
