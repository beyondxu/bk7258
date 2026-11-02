#include <os/os.h>
#include <os/str.h>
#include "os/mem.h"
#include "bk_posix.h"
#include "common.h"
#include "ui_resource.h"

//图片bin个数
#define UI_IMAGE_BIN_COUNT      (sizeof(g_ui_resource_infor)/sizeof(g_ui_resource_infor[0]))

// UI资源信息数组定义，存储所有需要在界面中显示的图片资源信息
// 每个元素包含图片名称、像素格式、尺寸、文件路径和LVGL图片数据结构
ui_resource_infor_t g_ui_resource_infor[] = {
    {IMG_BACKGROUND_NAME,      PIXEL_FMT_RGB565_LE, 480, 480, VFS_MOUNT_PATH"/img_bin/1_jpg.bin",      {{0}}},
    {IMG_ICON_NAME,            PIXEL_FMT_ARGB8888,  100, 100, VFS_MOUNT_PATH"/img_bin/icon_png.bin",   {{0}}},
};

static s32 _pre_load_image(int index)
{
    bk_err_t ret = BK_FAIL;
    lv_img_dsc_t *img_dsc = NULL;
    pixel_format_t   fmt;

    do{
        if(index+1 > UI_IMAGE_BIN_COUNT)
        {
            RES_LOGE("[%s][%d] index is invalid:%d, count:%d\r\n", __FUNCTION__, __LINE__, index, UI_IMAGE_BIN_COUNT);
            break;
        }

        img_dsc = &g_ui_resource_infor[index].img_data;
        if(0 != img_dsc->header.w)
        {
            RES_LOGE("[%s][%d] image %s have loaded\r\n", __FUNCTION__, __LINE__, g_ui_resource_infor[index].image_name);
            ret = BK_OK;
            break;
        }

        fmt = g_ui_resource_infor[index].fmt;
        if(PIXEL_FMT_RGB565_LE != fmt && PIXEL_FMT_ARGB8888 != fmt)
        {
            RES_LOGE("[%s][%d] image %s fmt is invalid:%d\r\n", __FUNCTION__, __LINE__, g_ui_resource_infor[index].image_name, fmt);
            ret = BK_FAIL;
            break;
        }

        img_dsc->header.w = g_ui_resource_infor[index].width;
        img_dsc->header.h = g_ui_resource_infor[index].height;
        img_dsc->header.cf = (fmt==PIXEL_FMT_RGB565_LE) ? LV_IMG_CF_TRUE_COLOR:LV_IMG_CF_TRUE_COLOR_ALPHA;
        img_dsc->data_size = img_dsc->header.w * img_dsc->header.h * 2;

        if(PIXEL_FMT_ARGB8888 == fmt) img_dsc->data_size = img_dsc->data_size*2;

        img_dsc->data = psram_malloc(img_dsc->data_size);
        if(!img_dsc->data)
        {
            RES_LOGE("[%s][%d] malloc fail:%d\r\n", __FUNCTION__, __LINE__, img_dsc->data_size);
            ret = BK_FAIL;
            break;
        }

        int fd = open(g_ui_resource_infor[index].image_file_path, O_RDONLY);
        if(fd < 0)
        {
            RES_LOGE("[%s][%d] image %s don't exist\r\n", __FUNCTION__, __LINE__, g_ui_resource_infor[index].image_file_path);
            ret = BK_FAIL;
            break;
        }

        read(fd, (void *)img_dsc->data, img_dsc->data_size);
        close(fd);
        ret = BK_OK;
    }while(0);

    if(BK_OK != ret && img_dsc->data)
    {
        psram_free((void *)img_dsc->data);
        img_dsc->data = NULL;
        os_memset(img_dsc, 0, sizeof(lv_img_dsc_t));
    }

    return ret;
}

/**
 * @brief 根据图片名称获取LVGL图片数据结构
 * 
 * 通过图片名称查找对应的图片资源，并返回LVGL图片数据结构指针，用于在LVGL界面中显示图片。
 * 
 * @param image_name 图片名称，用于标识要获取的图片资源
 * @return 成功返回LVGL图片数据结构指针，失败返回NULL
 */
lv_img_dsc_t *ui_resource_get_image_data(const char *image_name)
{
    int i = 0;
    lv_img_dsc_t *img_dsc = NULL;

    for(i=0; i<UI_IMAGE_BIN_COUNT; i++)
    {
        if(0 == os_strcmp(image_name, g_ui_resource_infor[i].image_name))
        {
            img_dsc = &g_ui_resource_infor[i].img_data;
            // 检查图片是否已加载
            if(0 == img_dsc->header.w)
            {
                _pre_load_image(i);
            }

            RES_LOGD("[%s][%d] data_size:%d, width:%d, height:%d, cf:%d\r\n", 
                                                                                __FUNCTION__, 
                                                                                __LINE__, 
                                                                                img_dsc->data_size,
                                                                                img_dsc->header.w, 
                                                                                img_dsc->header.h,
                                                                                img_dsc->header.cf);
            return &g_ui_resource_infor[i].img_data;
            break;
        }
    }

    return NULL;
}

/**
 * @brief 根据图片名称释放LVGL图片数据结构占用的内存
 * 
 * 通过图片名称查找对应的图片资源，并释放LVGL图片数据结构占用的内存。
 * 
 * @param image_name 图片名称，用于标识要释放的图片资源
 * @return 成功返回BK_OK，失败返回BK_FAIL
 */
s32 ui_resource_free_image_data(const char *image_name)
{
    int i = 0;
    lv_img_dsc_t *img_dsc = NULL;
    bk_err_t ret = BK_FAIL;

    for(i=0; i<UI_IMAGE_BIN_COUNT; i++)
    {
        if(0 == os_strcmp(image_name, g_ui_resource_infor[i].image_name))
        {
            img_dsc = &g_ui_resource_infor[i].img_data;
            // 检查图片是否已加载
            if(0 != img_dsc->header.w)
            {
                psram_free((void *)img_dsc->data);
                img_dsc->data = NULL;
                os_memset(img_dsc, 0, sizeof(lv_img_dsc_t));
            }

            ret = BK_OK;
            break;
        }
    }

    return ret;
}

/**
 * @brief 预加载ui需显示的图片，后续ui用时，不用解码，可提升帧率
 * @return 成功返回BK_OK
 */
s32 ui_resource_preload(void)
{
    int i = 0;
    lv_img_dsc_t *img_dsc = NULL;

    for(i=0; i<UI_IMAGE_BIN_COUNT; i++)
    {
        img_dsc = &g_ui_resource_infor[i].img_data;
        if(0 == img_dsc->header.w)
        {
            if(BK_OK == _pre_load_image(i))
            {
                RES_LOGI("[%s][%d] image %s pre load success, width:%d, height:%d\r\n", 
                                                                                __FUNCTION__, 
                                                                                __LINE__, 
                                                                                g_ui_resource_infor[i].image_name,
                                                                                img_dsc->header.w, 
                                                                                img_dsc->header.h);
            }
            else
            {
                RES_LOGE("[%s][%d] image %s pre load fail\r\n", __FUNCTION__, __LINE__, g_ui_resource_infor[i].image_name);
            }
        }
    }

    return BK_OK;
}
