#pragma once

#include "os/os.h"
#include "components/media_types.h"
#include "jpeg_hw_decode.h"

#define ANIM_TAG "anim"

#define ANIM_LOGI(...) BK_LOGI(ANIM_TAG, ##__VA_ARGS__)
#define ANIM_LOGE(...) BK_LOGE(ANIM_TAG, ##__VA_ARGS__)
#define ANIM_LOGD(...) BK_LOGD(ANIM_TAG, ##__VA_ARGS__)
#define ANIM_LOGV(...) BK_LOGV(ANIM_TAG, ##__VA_ARGS__)

/**
 * @brief 解码输出的编码格式枚举
 */
typedef enum {
    ANIMATION_DECODE_FORMAT_YUV422 = 0,       /*!< YUV422格式 */
    ANIMATION_DECODE_FORMAT_RGB565_LITTLE,    /*!< RGB565小端格式 */
    ANIMATION_DECODE_FORMAT_RGB565_BIG,       /*!< RGB565大端格式 */
    ANIMATION_DECODE_FORMAT_MAX               /*!< 格式数量最大值 */
} animation_decode_format_e;

/**
 * @brief 消息队列消息类型枚举
 */
typedef enum {
    ANIMATION_MSG_IDLE = 0,   /*!< 空闲消息 - 动画播放完成时触发 */
    ANIMATION_MSG_START,      /*!< 开始消息 - 外部调用开始接口时触发 */
    ANIMATION_MSG_EXIT        /*!< 退出消息 - 外部调用反初始化接口时触发 */
} animation_msg_type_e;

/**
 * @brief 融合回调函数类型
 * @param frame_data 解码后的帧数据
 * @param width 帧宽度
 * @param height 帧高度
 * @param user_data 用户自定义数据
 */
typedef void (*animation_blend_cb_t)(frame_buffer_t *bg_frame, void *user_data);

/**
 * @brief LCD显示回调函数类型
 * @param frame_data 解码后的帧数据
 * @param width 帧宽度
 * @param height 帧高度
 * @param user_data 用户自定义数据
 */
typedef void (*animation_lcd_cb_t)(frame_buffer_t *display_frame, void *user_data);

/**
 * @brief 动画播完回调函数类型
 * @param user_data 用户自定义数据
 */
typedef void (*animation_finish_cb_t)(void *user_data);

/**
 * @brief 动画模块配置结构体
 */
typedef struct {
    jpeg_hw_decode_config_t jpeg_hw_decode_config;   /*!< jpeg原始图片、解码所用到的内存申请及释放接口 */
    animation_decode_format_e decode_format;   /*!< 解码输出的编码格式 */
    animation_blend_cb_t blend_cb;             /*!< 融合回调函数 */
    animation_lcd_cb_t lcd_cb;                 /*!< LCD显示回调函数 */
    animation_finish_cb_t finish_cb;           /*!< 动画完成回调函数 */
    void *user_data;                           /*!< 用户自定义数据 */
} animation_module_config_t;

/**
 * @brief 动画播放参数结构体
 */
typedef struct {
    const char *file_prefix;   /*!< 图片文件名前缀 */
    uint32_t frame_count;      /*!< 图片张数 */
    uint32_t repeat_count;     /*!< 播放次数 */
    bool is_loop;              /*!< 是否循环播放 */
} animation_play_param_t;

/**
 * @brief 动画消息结构体
 */
typedef struct {
    animation_msg_type_e msg_type;         /*!< 消息类型 */
    animation_play_param_t play_param;     /*!< 播放参数，仅在消息类型为ANIMATION_MSG_START时有效 */
} animation_msg_t;

/**
 * @brief 动画模块初始化
 * @param config 动画模块配置参数
 * @return 成功返回0，失败返回负数
 */
int animation_module_init(const animation_module_config_t *config);

/**
 * @brief 开始播放动画
 * @param file_prefix 图片文件名前缀
 * @param frame_count 图片张数
 * @param repeat_count 播放次数
 * @param is_loop 是否循环播放
 * @return 成功返回0，失败返回负数
 */
int animation_start_play(const char *file_prefix, uint32_t frame_count, uint32_t repeat_count, bool is_loop);

/**
 * @brief 停止并反初始化动画模块
 * @return 成功返回0，失败返回负数
 */
int animation_module_deinit(void);
