#pragma once

#define HTTP_HEADER_BUFF_SIZE          1024       //http头buffer大小
#define RECV_BUFF_SIZE                 1024       //接收数据buffer大小

/**
 * @brief 获取文件的总长度，需要调用download_file_by_webclient或download_file_with_breakppoint_by_webclient成功后，才能调用此接口
 * 
 * 返回当前下载文件的总长度，该长度由download_file_by_webclient或
 * download_file_with_breakppoint_by_webclient函数在下载过程中设置。
 * 
 * @return 已下载文件的总长度，若未开始下载或获取失败则返回-1
 */
int download_file_len_get(void);

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
int download_file_by_webclient(const char *uri, int (*handle_function)(char *buffer, int size));

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
int download_file_with_breakppoint_by_webclient(const char *uri, uint32_t start_pos, int per_pack_size, int (*handle_function)(char *buffer, int size));
