#include <string.h>
#include <stdio.h>
#include <stdlib.h>  // 提供内存分配/释放函数
#include "os/os.h"  // 提供RTOS相关功能
#include "jpeg_file_animation.h"

// 模块配置参数
#define ANIMATION_QUEUE_SIZE         10      // 消息队列大小
#define ANIMATION_THREAD_PRIORITY    4       // 动画线程优先级
#define ANIMATION_THREAD_STACK_SIZE  2048    // 动画线程栈大小
#define ANIMATION_THREAD_NAME        "anim_thread" // 动画线程名称
#define ANIMATION_FILE_NAME_MAX_LEN  256     // 文件名最大长度
#define ANIMATION_TIMEOUT_FOREVER    0xFFFFFFFF  // 永久等待
#define ANIMATION_TIMEOUT_SHORT      100     // 短时间等待(ms)

// 动画模块内部状态结构体
typedef struct {
    bool is_init;                        // 初始化标志
    jpeg_hw_decode_config_t jpeg_hw_decode_config;         /*!< jpeg原始图片、解码所用到的内存申请及释放接口 */
    animation_decode_format_e decode_format;  // 解码格式
    animation_blend_cb_t blend_cb;       // 融合回调
    animation_lcd_cb_t lcd_cb;           // LCD显示回调
    animation_finish_cb_t finish_cb;     /*!< 动画完成回调函数 */
    void *user_data;                     // 用户数据
    
    // OS相关资源
    beken_thread_t thread_handle;        // 线程句柄
    beken_queue_t msg_queue;                   // 消息队列
    beken_semaphore_t exit_sem;          // 退出信号量
    
    // 播放状态
    animation_play_param_t current_play_param;  // 当前播放参数
} animation_module_internal_t;

// 全局动画模块实例
static animation_module_internal_t g_animation_module = {
    .is_init = false
};

/**
 * @brief JPEG图片解码函数
 * @param file_path 图片文件路径
 * @param frame_data 输出的帧数据缓冲区
 * @param width 输出的图片宽度
 * @param height 输出的图片高度
 * @return 成功返回0，失败返回负数
 */
static int animation_decode_jpeg(const char *file_path, frame_buffer_t **decode_buffer)
{
    int ret = -1;
    
    do {
        // 检查参数
        if (file_path == NULL) 
        {
            ANIM_LOGE("[%s][%d] Invalid parameters for JPEG decoding\r\n", __func__, __LINE__);
            ret = -1;
            break;
        }
        
        // 根据解码格式选择不同的解码函数
        switch (g_animation_module.decode_format)
        {
            case ANIMATION_DECODE_FORMAT_YUV422:
                ret = jpeg_hw_decode_from_file_to_yuv422(&g_animation_module.jpeg_hw_decode_config, file_path, decode_buffer);
                break;

            case ANIMATION_DECODE_FORMAT_RGB565_LITTLE:
                ret = jpeg_hw_decode_from_file_to_rgb565(&g_animation_module.jpeg_hw_decode_config, file_path, decode_buffer);
                break;

            case ANIMATION_DECODE_FORMAT_RGB565_BIG:
                ret = jpeg_hw_decode_from_file_to_rgb565(&g_animation_module.jpeg_hw_decode_config, file_path, decode_buffer);
                break;
            
            default:
                ANIM_LOGE("[%s][%d] Unsupported decode format: %d\r\n", __func__, __LINE__, g_animation_module.decode_format);
                ret = -1;
                break;
        }
    } while (0);
    
    return ret;
}

/**
 * @brief 动画解码线程函数
 * @param arg 线程参数
 */
static void animation_decode_thread(void *arg)
{
    animation_msg_t msg;
    bk_err_t ret = BK_OK;
    
    bool animation_running = false; // 运行状态标志
    uint32_t queue_timeout = ANIMATION_TIMEOUT_FOREVER; // 队列等待时间变量
    
    // 动画播放相关状态
    animation_play_param_t current_play_param = {0};
    uint32_t current_frame_idx = 0;
    uint32_t repeat_idx = 0;
    
    while (1)
    {
        // 从消息队列中获取消息
        ret = rtos_pop_from_queue(&g_animation_module.msg_queue, &msg, queue_timeout);
        if (ret == BK_OK)
        {
            // 成功获取消息，进行处理
            switch (msg.msg_type)
            {
                case ANIMATION_MSG_START:
                    // 设置运行状态和播放参数
                    animation_running = true;
                    queue_timeout = 0; // 运行时等待时间设为0
                    current_play_param = msg.play_param;
                    g_animation_module.current_play_param = msg.play_param;
                    
                    // 重置播放状态
                    current_frame_idx = 0;
                    repeat_idx = 0;
                    break;
                
                case ANIMATION_MSG_IDLE:
                    // 空闲状态，等待新的开始消息
                    ANIM_LOGI("[%s][%d] Animation play over\r\n", __func__, __LINE__);
                    if(g_animation_module.finish_cb)
                    {
                        g_animation_module.finish_cb(g_animation_module.user_data);
                    }
                    break;
                
                case ANIMATION_MSG_EXIT:
                    // 退出线程
                    ANIM_LOGI("[%s][%d] Animation thread received exit message, exiting...\r\n", __func__, __LINE__);
                    rtos_set_semaphore(&g_animation_module.exit_sem);
                    return;
                
                default:
                    ANIM_LOGE("[%s][%d] Unknown message type: %d\r\n", __func__, __LINE__, msg.msg_type);
                    break;
            }
        }
        
        // 当处于运行状态时，执行动画播放主逻辑
        if (animation_running)
        {
            // 处理当前帧
            if (current_frame_idx < current_play_param.frame_count)
            {
                // 构建文件名: prefix + 序号
                char file_path[ANIMATION_FILE_NAME_MAX_LEN];
                frame_buffer_t *decode_buffer = NULL;
                
                // 确保文件名不会溢出
                snprintf(file_path, sizeof(file_path) - 1, "%s%d.jpg", 
                         current_play_param.file_prefix, current_frame_idx);
                file_path[sizeof(file_path) - 1] = '\0';
                
                // 解码JPEG图片
                int decode_ret = animation_decode_jpeg(file_path, &decode_buffer);
                
                if (decode_ret == 0)
                {
                    // 调用融合回调
                    if (g_animation_module.blend_cb != NULL)
                    {
                        g_animation_module.blend_cb(decode_buffer, g_animation_module.user_data);
                    }
                    
                    // 调用LCD显示回调
                    if (g_animation_module.lcd_cb != NULL)
                    {
                        g_animation_module.lcd_cb(decode_buffer, g_animation_module.user_data);
                    }
                }
                
                // 移动到下一帧
                current_frame_idx++;
            }
            else
            {
                // 当前循环播放完成
                repeat_idx++;
                ANIM_LOGI("[%s][%d] %d times play over\r\n", __func__, __LINE__, repeat_idx);
                
                // 检查播放次数
                if (current_play_param.repeat_count > 0 && repeat_idx >= current_play_param.repeat_count)
                {
                    if (!current_play_param.is_loop)
                    {
                        // 播放完成，重置运行状态标志并发送IDLE消息
                        animation_running = false;
                        queue_timeout = ANIMATION_TIMEOUT_FOREVER;
                        
                        animation_msg_t idle_msg;
                        idle_msg.msg_type = ANIMATION_MSG_IDLE;
                        ret = rtos_push_to_queue(&g_animation_module.msg_queue, &idle_msg, ANIMATION_TIMEOUT_SHORT);
                        if (ret != BK_OK)
                        {
                            ANIM_LOGE("[%s][%d] Warning: Failed to send idle message\r\n", __func__, __LINE__);
                        }
                        
                        continue; // 跳出当前循环，等待新消息
                    }
                    else
                    {
                        repeat_idx = 0;  // 循环播放，重置计数
                    }
                }
                
                // 重置帧索引，开始新的一轮播放
                current_frame_idx = 0;
            }
        }
        else
        {
            // 非运行状态，使用无限等待时间
            queue_timeout = ANIMATION_TIMEOUT_FOREVER;
        }
    }
}

/**
 * @brief 动画模块初始化
 * @param config 动画模块配置参数
 * @return 成功返回0，失败返回负数
 */
int animation_module_init(const animation_module_config_t *config)
{
    bk_err_t ret = BK_OK;
    int result = 0;
    
    do {
        // 检查是否已初始化
        if (g_animation_module.is_init)
        {
            ANIM_LOGE("[%s][%d] Animation module already initialized\r\n", __FUNCTION__, __LINE__);
            break;
        }
        
        // 检查配置参数
        if (config == NULL)
        {
            ANIM_LOGE("[%s][%d] Invalid config parameter\r\n", __func__, __LINE__);
            result = -1;
            break;
        }
        
        // 保存配置
        os_memcpy(&g_animation_module.jpeg_hw_decode_config, &config->jpeg_hw_decode_config, sizeof(jpeg_hw_decode_config_t));
        g_animation_module.decode_format = config->decode_format;
        g_animation_module.blend_cb = config->blend_cb;
        g_animation_module.lcd_cb = config->lcd_cb;
        g_animation_module.finish_cb = config->finish_cb;
        g_animation_module.user_data = config->user_data;
        
        // 初始化消息队列
        ret = rtos_init_queue(&g_animation_module.msg_queue, "animation_queue", 
                              sizeof(animation_msg_t), ANIMATION_QUEUE_SIZE);
        if (ret != BK_OK) 
        {
            ANIM_LOGE("[%s][%d] Failed to initialize message queue\r\n", __func__, __LINE__);
            result = -2;
            break;
        }
        
        // 初始化退出信号量
        ret = rtos_init_semaphore(&g_animation_module.exit_sem, 1);
        if (ret != BK_OK) 
        {
            ANIM_LOGE("[%s][%d] Failed to initialize exit semaphore\r\n", __func__, __LINE__);
            result = -3;
            break;
        }
        
        // 创建动画解码线程
        ret = rtos_create_thread(&g_animation_module.thread_handle, 
                                ANIMATION_THREAD_PRIORITY, 
                                ANIMATION_THREAD_NAME,
                                animation_decode_thread,
                                ANIMATION_THREAD_STACK_SIZE, 
                                NULL);
        if (ret != BK_OK) 
        {
            ANIM_LOGE("[%s][%d] Failed to create animation decode thread\r\n", __func__, __LINE__);
            result = -4;
            break;
        }
        
        // 设置初始化标志
        g_animation_module.is_init = true;
        ANIM_LOGI("[%s][%d] Animation module initialized successfully\r\n", __func__, __LINE__);
        
    } while (0);
    
    // 资源清理
    if (result != 0)
    {
        if (g_animation_module.msg_queue)
        {
            rtos_deinit_queue(&g_animation_module.msg_queue);
        }

        if (g_animation_module.exit_sem)
        {
            rtos_deinit_semaphore(&g_animation_module.exit_sem);
        }
    }
    
    return result;
}

/**
 * @brief 开始播放动画
 * @param file_prefix 图片文件名前缀
 * @param frame_count 图片张数
 * @param repeat_count 播放次数
 * @param is_loop 是否循环播放
 * @return 成功返回0，失败返回负数
 */
int animation_start_play(const char *file_prefix, uint32_t frame_count, uint32_t repeat_count, bool is_loop)
{
    bk_err_t ret = BK_OK;
    int result = 0;
    
    do {
        // 检查是否已初始化
        if (!g_animation_module.is_init)
        {
            ANIM_LOGE("[%s][%d] Animation module not initialized\r\n", __func__, __LINE__);
            result = -1;
            break;
        }
        
        // 检查参数
        if (file_prefix == NULL || frame_count == 0)
        {
            ANIM_LOGE("[%s][%d] Invalid parameters\r\n", __func__, __LINE__);
            result = -2;
            break;
        }
        
        // 构造开始消息
        animation_msg_t msg;
        msg.msg_type = ANIMATION_MSG_START;
        msg.play_param.file_prefix = file_prefix;
        msg.play_param.frame_count = frame_count;
        msg.play_param.repeat_count = repeat_count;
        msg.play_param.is_loop = is_loop;
        
        // 发送消息到队列
        ret = rtos_push_to_queue(&g_animation_module.msg_queue, &msg, ANIMATION_TIMEOUT_SHORT);
        if (ret != BK_OK)
        {
            ANIM_LOGE("[%s][%d] Failed to push start message to queue\r\n", __func__, __LINE__);
            result = -3;
            break;
        }
        
        ANIM_LOGI("[%s][%d] Animation play started: prefix=%s, frames=%d, repeat=%d, loop=%d\r\n", __func__, __LINE__, 
               file_prefix, frame_count, repeat_count, is_loop);
        
    } while (0);
    
    return result;
}

/**
 * @brief 反初始化动画模块
 * @return 成功返回0，失败返回负数
 */
int animation_module_deinit(void)
{
    bk_err_t ret = BK_OK;
    int result = 0;
    
    do {
        // 检查是否已初始化
        if (!g_animation_module.is_init)
        {
            ANIM_LOGE("[%s][%d] Animation module not initialized\r\n", __func__, __LINE__);
            break;
        }
        
        // 发送退出消息
        animation_msg_t msg;
        msg.msg_type = ANIMATION_MSG_EXIT;
        ret = rtos_push_to_queue(&g_animation_module.msg_queue, &msg, ANIMATION_TIMEOUT_SHORT);
        if (ret != BK_OK)
        {
            ANIM_LOGE("[%s][%d] Failed to push exit message to queue\r\n", __func__, __LINE__);
            result = -1;
            // 即使发送消息失败，也要尝试清理资源
        }
        
        // 等待线程退出
        ret = rtos_get_semaphore(&g_animation_module.exit_sem, ANIMATION_TIMEOUT_FOREVER);
        if (ret != BK_OK)
        {
            ANIM_LOGE("[%s][%d] Failed to wait for thread exit\r\n", __func__, __LINE__);
            // 即使等待失败，也尝试清理资源
        }
        
    } while (0);
    
    // 无论前面是否成功，都尝试释放所有资源
    // 删除线程
    ret = rtos_delete_thread(&g_animation_module.thread_handle);
    if (ret != BK_OK)
    {
        ANIM_LOGE("[%s][%d] Warning: Failed to delete thread\r\n", __func__, __LINE__);
    }
    
    // 反初始化信号量
    ret = rtos_deinit_semaphore(&g_animation_module.exit_sem);
    if (ret != BK_OK)
    {
        ANIM_LOGE("[%s][%d] Warning: Failed to deinitialize semaphore\r\n", __func__, __LINE__);
    }
    
    // 反初始化消息队列
    ret = rtos_deinit_queue(&g_animation_module.msg_queue);
    if (ret != BK_OK)
    {
        ANIM_LOGE("[%s][%d] Warning: Failed to deinitialize message queue\r\n", __func__, __LINE__);
    }
    
    // 重置模块状态
    memset(&g_animation_module, 0, sizeof(g_animation_module));
    ANIM_LOGI("[%s][%d] Animation module deinitialized successfully\r\n", __func__, __LINE__);
    
    return result;
}