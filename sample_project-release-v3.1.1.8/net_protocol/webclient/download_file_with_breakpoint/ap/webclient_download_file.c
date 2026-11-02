#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include <bk_private/bk_wifi.h>
#include <lwip/inet.h>
#include <components/webclient.h>
#include "webclient_download_file.h"

#define TAG "webclient"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static int g_filelen = -1;

extern int webclient_shard_head_function(struct webclient_session *session, const char *URI, int *length);
extern int *webclient_register_shard_position_function(struct webclient_session *session, int (*handle_function)(char *buffer, int size));
extern int webclient_shard_position_function(struct webclient_session *session, const char *URI, int start, int length, int mem_size);

/**
 * @brief 获取文件的总长度，需要调用download_file_by_webclient或download_file_with_breakppoint_by_webclient成功后，才能调用此接口
 * 
 * 返回当前下载文件的总长度，该长度由download_file_by_webclient或
 * download_file_with_breakppoint_by_webclient函数在下载过程中设置。
 * 
 * @return 已下载文件的总长度，若未开始下载或获取失败则返回-1
 */
int download_file_len_get(void)
{
    return g_filelen;
}

/**
 * @brief 通过webclient下载http或https文件（连续下载模式）
 * 
 * 使用HTTP GET请求通过webclient接口下载文件，支持普通HTTP响应和chunked传输模式（chucked模式需用户自己处理，如包头包含文件长度，用来判断文件结尾）。
 * 下载的数据通过回调函数返回给调用者进行处理。
 * 
 * @param uri 下载文件的URI地址，支持http或https协议
 * @param handle_function 下载数据的处理回调函数
 *        @param buffer 下载到的数据缓冲区
 *        @param size 下载到的数据长度
 *        @return 处理结果，0表示成功
 * @return 成功返回BK_OK，失败返回错误码
 *         - BK_ERR_PARAM: 参数无效
 *         - BK_ERR_NO_MEM: 内存分配失败
 *         - BK_ERR_STATE: HTTP请求失败
 */
int download_file_by_webclient(const char *uri, int (*handle_function)(char *buffer, int size))
{
    struct webclient_session* session = NULL;
    unsigned char *buffer = NULL;
    int ret = 0;
    int bytes_read, resp_status;
    int content_length = -1;

    if(!uri || !handle_function)
    {
        LOGE("[%s][%d] param is invalid\r\n", __FUNCTION__, __LINE__);
        ret = BK_ERR_PARAM;
        goto __exit;
    }
    
    buffer = (uint8 *) psram_malloc(RECV_BUFF_SIZE);
    if (buffer == NULL)
    {
        LOGE("[%s][%d] no memory for receive buffer.\r\n", __FUNCTION__, __LINE__);
        ret = BK_ERR_NO_MEM;
        goto __exit;
    }

    /* create webclient session and set header response size */
    session = webclient_session_create(HTTP_HEADER_BUFF_SIZE);
    if (session == NULL)
    {
        LOGE("[%s][%d] no memory for http head.\r\n", __FUNCTION__, __LINE__);
        ret = BK_ERR_NO_MEM;
        goto __exit;
    }

    /* send GET request by default header */
    if ((resp_status = webclient_get(session, uri)) != 200)
    {
        LOGE("[%s][%d] webclient GET request failed, response(%d) error.\r\n", __FUNCTION__, __LINE__, resp_status);
        ret = BK_ERR_STATE;
        goto __exit;
    }

    // 获取文件长度
    content_length = webclient_content_length_get(session);
    g_filelen = content_length;
    if (content_length < 0)  //chunked type数据处理
    {
        LOGE("[%s][%d] webclient GET request type is chunked.\r\n", __FUNCTION__, __LINE__);
        do
        {
            bytes_read = webclient_read(session, (void *)buffer, RECV_BUFF_SIZE);
            if (bytes_read <= 0)
            {
                break;
            }

            handle_function((char *)buffer, bytes_read);    //通过回调上送数据

            // TODO  服务器如果是这种方式，建议文件增加包头包含文件长度，然后判断是否下载完成
        } while (1);
    }
    else
    {
        int content_pos = 0;

        LOGI("[%s][%d] file_len:%d\r\n", __FUNCTION__, __LINE__, content_length);
        do
        {
            bytes_read = webclient_read(session, (void *)buffer,
                    content_length - content_pos > RECV_BUFF_SIZE ?
                            RECV_BUFF_SIZE : content_length - content_pos);
            if (bytes_read <= 0)
            {
                break;
            }

            handle_function((char *)buffer, bytes_read);      //通过回调上送数据
            content_pos += bytes_read;

            if(content_pos >= content_length)
            {
                LOGI("[%s][%d] webclient download file success\r\n", __FUNCTION__, __LINE__);
                ret = BK_OK;
            }
        } while (content_pos < content_length);
    }

__exit:
    if (session)
    {
        webclient_close(session);
    }

    if (buffer)
    {
        web_free(buffer);
    }

    g_filelen = -1;

    return ret;
}

/**
 * @brief 通过webclient下载http或者https文件（断点续传模式）
 * 
 * 使用HTTP断点续传功能通过webclient接口下载文件，支持从指定位置开始下载。
 * 下载的数据通过回调函数返回给调用者进行处理，适用于大文件下载或网络不稳定的场景。
 * 
 * @param uri 下载文件的URI地址，支持http或https    
 * @param start_pos 开始下载的位置（文件偏移量）
 * @param per_pack_size 每次下载的数据块大小
 * @param handle_function 下载数据的处理回调函数
 *        @param buffer 下载到的数据缓冲区
 *        @param size 下载到的数据长度
 *        @return 处理结果，0表示成功
 * @return 成功返回WEBCLIENT_OK，失败返回错误码
 *         - BK_ERR_PARAM: 参数无效
 *         - BK_ERR_NO_MEM: 内存分配失败
 *         - BK_ERR_NOT_FOUND: 无法获取文件长度
 */
int download_file_with_breakppoint_by_webclient(const char *uri, uint32_t start_pos, int per_pack_size, int (*handle_function)(char *buffer, int size))
{
    struct webclient_session* session = NULL;
    bk_err_t ret = BK_OK;
    int length = 0;

    if(!uri || !handle_function)
    {
        LOGE("[%s][%d] param is invalid\r\n", __FUNCTION__, __LINE__);
        ret = BK_ERR_PARAM;
        goto __exit;
    }

    /* sometime, the header bufsz can set more smaller */
    session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ/4);
    if (session == NULL)
    {
        LOGE("[%s][%d] session create fail\r\n", __FUNCTION__, __LINE__);
        ret = -BK_ERR_NO_MEM;
        goto __exit;
    }

    /* get the real data length */
    webclient_shard_head_function(session, uri, &length);
    if(length <= 0)
    {
        LOGE("[%s][%d] url file length is %d\r\n", __FUNCTION__, __LINE__, length);
        ret = -BK_ERR_NOT_FOUND;
        goto __exit;
    }

    g_filelen = length;
    /* register the handle function, you can handle data in the function */
    webclient_register_shard_position_function(session, handle_function);

    /* the "memory size" that you can provide in the project and uri */
    ret = webclient_shard_position_function(session, uri, start_pos, length, per_pack_size);
    if(ret != WEBCLIENT_OK)
    {
        LOGE("[%s][%d] web shard download, test failed!\r\n", __FUNCTION__, __LINE__);
    }

    /* clear the handle function */
    webclient_register_shard_position_function(session, NULL);

__exit:
    if (session)
    {
        webclient_close(session);
        session = NULL;
    }

    g_filelen = -1;
    
    return ret;
}
