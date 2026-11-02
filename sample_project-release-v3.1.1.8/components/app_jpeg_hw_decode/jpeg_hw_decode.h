#pragma once

#include "components/media_types.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"


/**
 * @brief 解码配置输入参数
 * 
 * JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口，有以下两种示例可借鉴
 * Usage example1: 从系统frame_buffer申请内存，好处是不会因内存碎片引起内存申请失败
 *               //jpeg原始图片malloc
 *               static frame_buffer_t *_jpeg_frame_malloc(uint32 size)
 *               {
 *                   return frame_buffer_encode_malloc(size);
 *               }
 *
 *               //jpeg原始图片free
 *               static void _jpeg_frame_free(frame_buffer_t *frame)
 *               {
 *                   frame_buffer_encode_free(frame);
 *               }
 *
 *               //解码后图片malloc
 *               static frame_buffer_t *_decode_frame_malloc(uint32 size)
 *               {
 *                   return frame_buffer_display_malloc(size);
 *               }
 *
 *               //解码后图片free
 *               static void _decode_frame_free(frame_buffer_t *frame)
 *               {
 *                   frame_buffer_display_free(frame);
 *               }
 *
 *            Usage example2: 从系统psram堆申请内存，好处是可将所有psram当堆内存使用，坏处是如果应用代码使用内存较多，并且没即时释放，可能会导致因内存碎片引起内存申请失败
 *               //jpeg原始图片、解码图片malloc
 *               static frame_buffer_t *_frame_malloc(uint32 size)
 *               {
 *                   frame_buffer_t *frame = psram_malloc(sizeof(frame_buffer_t));
 *                  if(frame)
 *                  {
 *                      os_memset(frame, 0, sizeof(frame_buffer_t));
 *                      frame->frame = psram_malloc(size);
 *                       if(!frame->frame)
 *                      {
 *                          psram_free(frame);
 *                          frame = NULL;
 *                      }
 *                  }
 *
 *                   return frame;
 *                }
 *
 *               //jpeg原始图片、解码图片free
 *               static void _frame_free(frame_buffer_t *frame)
 *               {
 *                   if(frame)
 *                   {
 *                       if(frame->frame)
 *                       {
 *                           psram_free(frame->frame);
 *                       }
 *
 *                       psram_free(frame);
 *                   }
 *               }
 */
typedef struct{
    bk_jpeg_decode_hw_handle_t jpeg_decode_handle;                  /*!硬解码句柄，传NULL则由内部创建硬解码句柄 */

    frame_buffer_t *(*jpeg_frame_malloc)(uint32 size);              /*!给解码模块用来分配jpeg frame的callback */
    void (*jpeg_frame_free)(frame_buffer_t *frame);                 /*!给解码模块用来释放jpeg frame的callback */

    frame_buffer_t *(*decode_frame_malloc)(uint32 size);            /*!给解码模块用来分配decode frame的callback */
    void (*decode_frame_free)(frame_buffer_t *frame);               /*!给解码模块用来释放decode frame的callback */

//以下参数仅对转RGB565有效
    /*
    解码输出的宽度、高度，用于图片宽高不满足bk7258硬件要求时使用，填0内部会按实际图片的长宽输出。
    如: 需要解码jpeg图片，显示到240X240的lcd 屏上，宽240不是32倍数，需将图片做成256X240（新建256X240画布，拷贝240X240图片进来按左上角原点对齐），
        这里output_width、out_height填240、240，接口只会解码输出（0,0）到（240,240)区域的解码数据
    */
    uint16 output_width;
    uint16 output_height;

    bool byte_swap;                                                  /*!解码输出大端还是小端格式数据，false为小端， true为大端*/
    bool use_dma2d;                                                  /*!yuv转rgb565用dma2d还是软件算法 false 使用软件算法，true 使用dma2d算法*/
}jpeg_hw_decode_config_t;

/**
 * @brief 对文件系统的jpeg文件，利用JPEG硬解并输出YUV422格式
 * @param[in] jpeg_decode_config 输入参数，硬解文件句柄、JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口等
 * @param[in] file_name 输入参数，JPEG文件路径
 * @param[out] decode_frame 输出参数，用于存储解码后的帧缓冲区指针
 * @return bk_err_t 成功返回BK_OK，失败返回错误码
 */
bk_err_t jpeg_hw_decode_from_file_to_yuv422(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                       const char *file_name, 
                                                       frame_buffer_t **decode_frame);

/**
 * @brief 对文件系统的jpeg文件，利用JPEG硬解并输出RGB565格式
 * @param[in] jpeg_decode_config 输入参数，JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口
 * @param[in] file_name 输入参数，JPEG文件路径
 * @param[out] decode_frame 输出参数，用于存储解码后的帧缓冲区指针
 * @return bk_err_t 成功返回BK_OK，失败返回错误码
 */
bk_err_t jpeg_hw_decode_from_file_to_rgb565(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                       const char *file_name, 
                                                       frame_buffer_t **decode_frame);

/**
 * @brief 对jpeg内存数据，利用JPEG硬解并输出YUV422格式
 * @param[in] jpeg_decode_config 输入参数，硬解文件句柄、JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口等
 * @param[in] file_name 输入参数，JPEG文件内容数据
 * @param[out] decode_frame 输出参数，用于存储解码后的帧缓冲区指针
 * @return bk_err_t 成功返回BK_OK，失败返回错误码
 */
bk_err_t jpeg_hw_decode_from_buffer_to_yuv422(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                          frame_buffer_t *jpeg_frame, 
                                                          frame_buffer_t **decode_frame);

/**
 * @brief 对jpeg内存数据，利用JPEG硬解并输出RGB565格式
 * @param[in] jpeg_decode_config 输入参数，JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口
 * @param[in] jpeg_frame 输入参数，JPEG文件内容数据
 * @param[out] decode_frame 输出参数，用于存储解码后的帧缓冲区指针
 * @return bk_err_t 成功返回BK_OK，失败返回错误码
 */
bk_err_t jpeg_hw_decode_from_buffer_to_rgb565(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                          frame_buffer_t *jpeg_frame, 
                                                          frame_buffer_t **decode_frame);

