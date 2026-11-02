#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "bk_posix.h"
#include "components/media_types.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"
#include "jpeg_hw_decode.h"
#include "driver/dma2d.h"
#include "modules/image_scale.h"

#define TAG "hw_img"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bk_err_t _jpeg_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    if (result == BK_OK) 
    {
        LOGD("[%s][%d] jpeg decode success! format_type: %d, out_frame: %p\n", __func__, __LINE__, format_type, out_frame);
    } 
    else 
    {
        LOGE("[%s][%d] jpeg decode failed! format_type: %d, result: %d, out_frame: %p\n", __func__, __LINE__, format_type, result, out_frame);
    }

    return BK_OK;
}

static bk_jpeg_decode_hw_handle_t g_jpeg_decode_handle = NULL;
static bk_jpeg_decode_hw_config_t g_jpeg_decode_config = {
    .decode_cbs = {.out_complete = _jpeg_decode_complete,}
};

static inline uint16_t bswap16_self(uint16_t x)
{
    uint32_t result;
    __asm__ volatile (
        "eor   %1, %1, %1, ror #16 \n"
        "mov   %1, %1, ror #8      \n"
        : "=r" (result)
        : "0" ((uint32_t)x << 16)
    );

    return (uint16_t)(result >> 16);
}

//获取文件长度
static uint32 _get_filesize(const char *file_name)
{
    int fd = 0;
    uint32 file_size = 0;

    do{
        fd = open(file_name, O_RDONLY);
        if(fd < 0)
        {
            break;
        }

        lseek(fd, 0, SEEK_END);
        file_size = ftell(fd);
        close(fd);
    }while(0);

    return file_size;
}

//读出文件，并存在frame_buffer里
static frame_buffer_t *_read_jpeg_file(const jpeg_hw_decode_config_t *jpeg_decode_config, const char *file_name)
{
    uint32 file_size = 0;
    frame_buffer_t *jpeg_frame = NULL;
    bk_err_t ret = BK_FAIL;

    do{
        file_size = _get_filesize(file_name);
        if(0 == file_size)
        {
            LOGE("[%s][%d] file %s don't exist\r\n", __FUNCTION__, __LINE__, file_name);
            break;
        }

        jpeg_frame = jpeg_decode_config->jpeg_frame_malloc(file_size);
        if (!jpeg_frame) 
        {
            LOGE("[%s][%d] malloc fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        jpeg_frame->length = file_size;
        jpeg_frame->size = file_size;
        LOGI("[%s][%d] read b\r\n", __FUNCTION__, __LINE__);
        int fd = open(file_name, O_RDONLY);
        read(fd, jpeg_frame->frame, jpeg_frame->length);
        close(fd);
        LOGI("[%s][%d] read e\r\n", __FUNCTION__, __LINE__);
        ret = BK_OK;
    }while(0);

    if(BK_OK != ret && jpeg_frame)
    {
        jpeg_decode_config->jpeg_frame_free(jpeg_frame);
        jpeg_frame = NULL;
    }

    return jpeg_frame;
}

//硬解初始化
static bk_err_t _jpeg_hw_decode_init(void)
{
    bk_err_t ret = BK_FAIL;

    do{
        ret = bk_hardware_jpeg_decode_new(&g_jpeg_decode_handle, &g_jpeg_decode_config);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] jpeg hw new fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        ret = bk_jpeg_decode_hw_open(g_jpeg_decode_handle);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] jpeg hw open fail\r\n", __FUNCTION__, __LINE__);
            break;
        }
    }while(0);

    return ret;
}

//硬解反初始化
static bk_err_t _jpeg_hw_decode_deinit(void)
{
    bk_jpeg_decode_hw_close(g_jpeg_decode_handle);
    bk_jpeg_decode_hw_delete(g_jpeg_decode_handle);

    g_jpeg_decode_handle = NULL;

    return BK_OK;
}

//dma2d转换
static void _dma2d_yuyv2rgb565(frame_buffer_t *src_frame, frame_buffer_t *dst_frame, bool byte_swap)
{
    dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};

    bk_dma2d_driver_init();
    dma2d_memcpy_pfc.input_addr = (char *)src_frame->frame;
    dma2d_memcpy_pfc.output_addr = (char *)dst_frame->frame;
    dma2d_memcpy_pfc.mode = DMA2D_M2M_PFC;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_YUYV;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB565;
    dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.dma2d_width = dst_frame->width;
    dma2d_memcpy_pfc.dma2d_height = dst_frame->height;
    dma2d_memcpy_pfc.src_frame_width = src_frame->width;
    dma2d_memcpy_pfc.src_frame_height = src_frame->height;
    dma2d_memcpy_pfc.dst_frame_width = dst_frame->width;
    dma2d_memcpy_pfc.dst_frame_height = dst_frame->height;
    dma2d_memcpy_pfc.src_frame_xpos = 0;
    dma2d_memcpy_pfc.src_frame_ypos = 0;
    dma2d_memcpy_pfc.dst_frame_xpos = 0;
    dma2d_memcpy_pfc.dst_frame_ypos = 0;
    dma2d_memcpy_pfc.input_red_blue_swap = 0;
    dma2d_memcpy_pfc.output_red_blue_swap = 0;
    if (byte_swap) 
    {
        dma2d_memcpy_pfc.out_byte_by_byte_reverse = 1;
    }
    else
    {
        dma2d_memcpy_pfc.out_byte_by_byte_reverse = 0;
    }

    bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();
    while (bk_dma2d_is_transfer_busy())
    {
        rtos_delay_milliseconds(4);
    }
}

static bk_err_t _jpeg_hw_decode_to_yuv422(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                  frame_buffer_t *jpeg_frame, 
                                                  frame_buffer_t **decode_frame)
{
    frame_buffer_t *decode_frame_tmp = NULL;
    bk_err_t ret = BK_FAIL;
    bk_jpeg_decode_hw_handle_t jpeg_decode_hw_handle = NULL;
    bk_jpeg_decode_img_info_t img_info = {0};

    do{
        if(!jpeg_decode_config 
            || !jpeg_frame 
            || !jpeg_frame->frame 
            || 0 == jpeg_frame->length 
            || 0 == jpeg_frame->size)
        {
            LOGE("[%s][%d] param is NULL\r\n", __FUNCTION__, __LINE__);
            break;
        }

        //获取图片长、宽
        img_info.frame = jpeg_frame;
        ret = bk_get_jpeg_data_info(&img_info);
        if (BK_OK != ret) 
        {
            LOGE("[%s][%d] get jpeg info fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }
        
        if(( 0 != img_info.width % 32 ) || ( 0 != img_info.height % 8 ))
        {
            LOGE("[%s][%d] width isn't a multiple of 32 or height isn't a multiple of 8\r\n", __FUNCTION__, __LINE__);
            ret = BK_FAIL;
            break;
        }

        jpeg_frame->width = img_info.width;
        jpeg_frame->height = img_info.height;

        if(!jpeg_decode_config->decode_frame_malloc 
            || !jpeg_decode_config->decode_frame_free)
        {
            LOGE("[%s][%d] jpeg malloc fun is NULL:%p, %p\r\n", 
                                                                    __FUNCTION__, 
                                                                    __LINE__, 
                                                                    jpeg_decode_config->decode_frame_malloc,
                                                                    jpeg_decode_config->decode_frame_free);
            ret = BK_FAIL;
            break;
        }

        if(!jpeg_decode_config->jpeg_decode_handle)
        {
            LOGD("[%s][%d] hw decode init\r\n", __FUNCTION__, __LINE__);
            ret = _jpeg_hw_decode_init();
            if(BK_OK != ret)
            {
                LOGE("[%s][%d] hw decode init fail\r\n", __FUNCTION__, __LINE__);
                break;
            }

            jpeg_decode_hw_handle  = g_jpeg_decode_handle;
        }
        else
        {
            LOGD("[%s][%d] hw decode init by user\r\n", __FUNCTION__, __LINE__);
            jpeg_decode_hw_handle  = jpeg_decode_config->jpeg_decode_handle;
        }

        decode_frame_tmp = jpeg_decode_config->decode_frame_malloc(jpeg_frame->width * jpeg_frame->height * 2);
        if (!decode_frame_tmp) 
        {
            LOGE("[%s][%d] malloc fail\r\n", __FUNCTION__, __LINE__);
            ret = BK_FAIL;
            break;
        }

        decode_frame_tmp->width = jpeg_frame->width;
        decode_frame_tmp->height = jpeg_frame->height;
        decode_frame_tmp->length = jpeg_frame->width * jpeg_frame->height * 2;
        decode_frame_tmp->size = decode_frame_tmp->length;
        ret = bk_jpeg_decode_hw_decode(jpeg_decode_hw_handle, jpeg_frame, decode_frame_tmp);
        if (BK_OK != ret) 
        {
            LOGE("[%s][%d] hw decode start fail %d\n", __func__, __LINE__, ret);
            break;
        }

        if(!jpeg_decode_config->jpeg_decode_handle)
        {
            _jpeg_hw_decode_deinit();
        }

        decode_frame_tmp->fmt = PIXEL_FMT_YUYV;
        *decode_frame = decode_frame_tmp;
        ret = BK_OK;
    }while(0);

    if(BK_OK != ret)
    {
        if (decode_frame_tmp) 
        {
            jpeg_decode_config->decode_frame_free(decode_frame_tmp);
            decode_frame_tmp = NULL;
        }
    }

    return ret;
}

static bk_err_t _jpeg_hw_decode_to_rgb565(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                  frame_buffer_t *jpeg_frame, 
                                                  frame_buffer_t **decode_frame)
{
    bk_err_t ret = BK_FAIL;
    frame_buffer_t *decode_frame_tmp = NULL;
    frame_buffer_t *output_frame = NULL;
    uint16_t output_width = 0;
    uint16_t output_height = 0;

    do{
        // 解码输出yuv422
        ret = _jpeg_hw_decode_to_yuv422(jpeg_decode_config, jpeg_frame, &decode_frame_tmp);
        if(BK_OK != ret)
        {
            break;
        }

        if(jpeg_decode_config->output_width > decode_frame_tmp->width 
            || jpeg_decode_config->output_height > decode_frame_tmp->height)
        {
            LOGE("[%s][%d] output size is bigger than jpeg frame (%d, %d) > (%d, %d)\n", 
                                                                            __func__, 
                                                                            __LINE__, 
                                                                            jpeg_decode_config->output_width, 
                                                                            jpeg_decode_config->output_height, 
                                                                            decode_frame_tmp->width, 
                                                                            decode_frame_tmp->height);
            break;
        }

        if(0 == jpeg_decode_config->output_width)
        {
            output_width = decode_frame_tmp->width;
            output_height = decode_frame_tmp->height;
        }
        else
        {
            output_width = jpeg_decode_config->output_width;
            output_height = jpeg_decode_config->output_height;
            LOGI("[%s][%d] ready decode (%d, %d) to (%d, %d)\n", 
                                                    __func__, 
                                                    __LINE__, 
                                                    decode_frame_tmp->width, 
                                                    decode_frame_tmp->height, 
                                                    output_width, 
                                                    output_height);
        }

        output_frame = jpeg_decode_config->decode_frame_malloc(output_width * output_height * 2);
        if(!output_frame)
        {
            LOGE("[%s][%d] malloc fail\n", __func__, __LINE__);
            break;
        }

        output_frame->length = output_width * output_height * 2;
        output_frame->size = output_frame->length;
        output_frame->width = output_width;
        output_frame->height = output_height;

        LOGD("[%s][%d] decode before, use %s\r\n", __func__, __LINE__, jpeg_decode_config->use_dma2d ? "dma2d":"soft");
        if(jpeg_decode_config->use_dma2d)
        {
            // 使用dma2d，将yuv转成rgb565
            _dma2d_yuyv2rgb565(decode_frame_tmp, output_frame, jpeg_decode_config->byte_swap);
        }
        else
        {
            // 暂时不支持指定大小输出
            if(output_frame->width != decode_frame_tmp->width)
            {
                LOGE("[%s][%d] use soft func, need output width is equal to jpeg width\n", __func__, __LINE__);
                ret = BK_FAIL;
                break;
            }

            // 使用软件算法，将yuv转成rgb565
            yuyv_to_rgb565(decode_frame_tmp->frame, output_frame->frame, (int)output_frame->width,(int)output_frame->height);

            // 转为大端格式
            if (jpeg_decode_config->byte_swap) 
            {
                for (int i = 0; i < output_frame->length; i += 2) 
                {
                    uint16_t *p = (uint16_t *)&output_frame->frame[i];
                    *p = bswap16_self(*p);
                }
            }
        }

        output_frame->fmt = jpeg_decode_config->byte_swap ? PIXEL_FMT_RGB565_LE:PIXEL_FMT_RGB565;
        *decode_frame = output_frame;
        ret = BK_OK;
    }while(0);

    if (decode_frame_tmp) 
    {
        jpeg_decode_config->decode_frame_free(decode_frame_tmp);
        decode_frame_tmp = NULL;
    }

    if(BK_OK != ret)
    {
        if (output_frame) 
        {
            jpeg_decode_config->decode_frame_free(output_frame);
            output_frame = NULL;
        }
    }

    return ret;
}

bk_err_t jpeg_hw_decode_from_file_to_yuv422(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                       const char *file_name, 
                                                       frame_buffer_t **decode_frame)
{
    frame_buffer_t *jpeg_frame = NULL;
    bk_err_t ret = BK_FAIL;

    do{
        if(!jpeg_decode_config || !file_name)
        {
            LOGE("[%s][%d] param is NULL\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if(!jpeg_decode_config->jpeg_frame_malloc 
            || !jpeg_decode_config->jpeg_frame_free)
        {
            LOGE("[%s][%d] jpeg malloc fun is NULL:%p, %p, %p, %p\r\n", 
                                                                    __FUNCTION__, 
                                                                    __LINE__, 
                                                                    jpeg_decode_config->decode_frame_malloc,
                                                                    jpeg_decode_config->decode_frame_free);
            break;
        }

        // 从文件获取jpeg_frame
        jpeg_frame = _read_jpeg_file(jpeg_decode_config, file_name);
        if(!jpeg_frame)
        {
            ret = BK_FAIL;
            break;
        }

        // 解码为yuv422
        ret = _jpeg_hw_decode_to_yuv422(jpeg_decode_config, jpeg_frame, decode_frame);
        if(BK_OK == ret)
        {
            LOGI("[%s][%d] decode %s success, width:%d, height:%d, length:%d\n", 
                                                                           __func__, 
                                                                           __LINE__, 
                                                                           file_name, 
                                                                           (*decode_frame)->width, 
                                                                           (*decode_frame)->height, 
                                                                           (*decode_frame)->length);
        }
    }while(0);

    if (jpeg_frame) 
    {
        jpeg_decode_config->jpeg_frame_free(jpeg_frame);
        jpeg_frame = NULL;
    }

    return ret;
}

bk_err_t jpeg_hw_decode_from_file_to_rgb565(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                       const char *file_name, 
                                                       frame_buffer_t **decode_frame)
{
    frame_buffer_t *jpeg_frame = NULL;
    bk_err_t ret = BK_FAIL;

    do{
        if(!jpeg_decode_config || !file_name)
        {
            LOGE("[%s][%d] param is NULL\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if(!jpeg_decode_config->jpeg_frame_malloc 
            || !jpeg_decode_config->jpeg_frame_free)
        {
            LOGE("[%s][%d] jpeg malloc fun is NULL:%p, %p, %p, %p\r\n", 
                                                                    __FUNCTION__, 
                                                                    __LINE__, 
                                                                    jpeg_decode_config->decode_frame_malloc,
                                                                    jpeg_decode_config->decode_frame_free);
            break;
        }

        // 从文件获取jpeg_frame
        jpeg_frame = _read_jpeg_file(jpeg_decode_config, file_name);
        if(!jpeg_frame)
        {
            ret = BK_FAIL;
            break;
        }

        // 解码为rgb565
        ret = _jpeg_hw_decode_to_rgb565(jpeg_decode_config, jpeg_frame, decode_frame);
        if(BK_OK == ret)
        {
            LOGI("[%s][%d] decode %s success, width:%d, height:%d, length:%d\n", 
                                                                           __func__, 
                                                                           __LINE__, 
                                                                           file_name, 
                                                                           (*decode_frame)->width, 
                                                                           (*decode_frame)->height, 
                                                                           (*decode_frame)->length);
        }
    }while(0);

    if (jpeg_frame) 
    {
        jpeg_decode_config->jpeg_frame_free(jpeg_frame);
        jpeg_frame = NULL;
    }

    return ret;
}

bk_err_t jpeg_hw_decode_from_buffer_to_yuv422(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                          frame_buffer_t *jpeg_frame, 
                                                          frame_buffer_t **decode_frame)
{
    return _jpeg_hw_decode_to_yuv422(jpeg_decode_config, jpeg_frame, decode_frame);
}

bk_err_t jpeg_hw_decode_from_buffer_to_rgb565(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                          frame_buffer_t *jpeg_frame, 
                                                          frame_buffer_t **decode_frame)
{
    return _jpeg_hw_decode_to_rgb565(jpeg_decode_config, jpeg_frame, decode_frame);
}

