#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "frame_buffer_app.h"

#define USE_PSRAM_MALLOC 0

/**
 * @brief 给解码线程调用的callback，用于分配JPEG原始图片存储的内存
 * @param size jpeg原始图片内容长度
 * @return 成功返回分配的frame_buffer_t指针，失败返回NULL
 */
frame_buffer_t *jpeg_frame_malloc(uint32 size)
{
#if USE_PSRAM_MALLOC
    frame_buffer_t *frame = psram_malloc(sizeof(frame_buffer_t));
    if(frame)
    {
        os_memset(frame, 0, sizeof(frame_buffer_t));
        frame->frame = psram_malloc(size);
        if(!frame->frame)
        {
            psram_free(frame);
            frame = NULL;
        }
    }
#else
    frame_buffer_t *frame = frame_buffer_encode_malloc(size);
#endif

    if(frame)
    {
        frame->size = size;
        frame->length = size;
    }

    return frame;
}

/**
 * @brief 给解码线程调用的callback，用于释放JPEG原始图片存储的内存
 * @return 无
 */
void jpeg_frame_free(frame_buffer_t *frame)
{
#if USE_PSRAM_MALLOC
    if(frame)
    {
        if(frame->frame)
        {
            psram_free(frame->frame);
        }

        psram_free(frame);
    }
#else
    frame_buffer_encode_free(frame);
#endif
}

/**
 * @brief 给解码线程调用的callback，用于分配JPEG解码内容用来存储的内存
 * @param size 解码内存大小
 * @return 成功返回分配的frame_buffer_t指针，失败返回NULL
 */
frame_buffer_t *decode_frame_malloc(uint32 size)
{
#if USE_PSRAM_MALLOC
    frame_buffer_t *frame = psram_malloc(sizeof(frame_buffer_t));
    if(frame)
    {
        os_memset(frame, 0, sizeof(frame_buffer_t));
        frame->frame = psram_malloc(size);
        if(!frame->frame)
        {
            psram_free(frame);
            frame = NULL;
        }
    }
#else
    frame_buffer_t *frame = frame_buffer_display_malloc(size);
#endif

    if(frame)
    {
        frame->size = size;
        frame->length = size;
    }

    return frame;
}

/**
 * @brief 给解码线程调用的callback，用于释放JPEG解码图片存储的内存
 * @return 无
 */
void decode_frame_free(frame_buffer_t *frame)
{
#if USE_PSRAM_MALLOC
    if(frame)
    {
        if(frame->frame)
        {
            psram_free(frame->frame);
        }

        psram_free(frame);
    }
#else
    frame_buffer_display_free(frame);
#endif
}

/**
 * @brief 刷屏完成调用的callback，释放刷屏buffer的内存
 * @return bk_ok
 */
bk_err_t display_free_callback(void *args)
{
    frame_buffer_t *frame = (frame_buffer_t *)args;

    decode_frame_free(frame);

    return BK_OK;
}

