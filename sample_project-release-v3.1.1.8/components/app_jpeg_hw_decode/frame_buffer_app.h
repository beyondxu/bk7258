#pragma once

#include "frame_buffer.h"

/**
 * @brief 给jpeg解码模块调用，用于分配JPEG原始图片存储的内存
 * @param size jpeg原始图片内容长度
 * @return 成功返回分配的frame_buffer_t指针，失败返回NULL
 */
frame_buffer_t *jpeg_frame_malloc(uint32 size);

/**
 * @brief 给jpeg解码模块调用，用于释放JPEG原始图片存储的内存
 * @return 无
 */
void jpeg_frame_free(frame_buffer_t *frame);

/**
 * @brief 给jpeg解码模块调用，用于分配JPEG解码内容用来存储的内存
 * @param size 解码内存大小
 * @return 成功返回分配的frame_buffer_t指针，失败返回NULL
 */
frame_buffer_t *decode_frame_malloc(uint32 size);

/**
 * @brief 给jpeg解码模块调用，用于释放JPEG解码图片存储的内存
 * @return 无
 */
void decode_frame_free(frame_buffer_t *frame);

/**
 * @brief 刷屏完成调用的callback，释放刷屏buffer的内存
 * @return bk_ok
 */
bk_err_t display_free_callback(void *args);

