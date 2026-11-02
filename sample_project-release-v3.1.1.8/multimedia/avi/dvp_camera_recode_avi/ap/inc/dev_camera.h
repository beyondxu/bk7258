#ifndef __DEV_CAMERA_H__
#define __DEV_CAMERA_H__

#include "components/media_types.h"

#define DVP_POWER_PIN       GPIO_28
/**
 * @brief 打开DVP相机
 * @param width  图像宽度
 * @param height 图像高度
 * @param fps    帧率
 * @return 0 成功；负值 错误码
 */
int dev_dvp_camera_open(uint32_t width, uint32_t height, frame_fps_t fps);

/**
 * @brief 关闭DVP相机
 * @return 0 成功；负值 错误码
 */
int dev_dvp_camera_close(void);

#endif