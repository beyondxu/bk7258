#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <components/shell_task.h>
#include <modules/wifi.h>
#include <components/netif.h>
#include <components/event.h>
#include <string.h>
#include "driver/flash_partition.h"
#include "partitions_gen.h"
#include "bk_posix.h"
#include "ftp.h"

#define DEV_SD_CARD         0
#define DEV_LITTLEFS        1
#define FTP_VFS_DRIVER      DEV_SD_CARD

#define BK_EXAMPLE_LOG             BK_LOGE
#define TAG "ftp_server"
#define TEST_PASS() 	BK_EXAMPLE_LOG(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL() 	BK_EXAMPLE_LOG(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#define FTP_THREAD_PRIORITY     5
#define FTP_THREAD_STACK_SIZE   4 * 1024

#define CONFIG_EXAMPLE_WIFI_STA_SSID        "sample_project_wifi"
#define CONFIG_EXAMPLE_WIFI_STA_PASSWORD    "12345678"

static beken_thread_t ftp_server_handle = NULL;
static beken_semaphore_t g_wifi_cmd_sema = NULL;

// 处理网络接口事件的回调函数
static int wifi_netif_event_cb(void *arg, event_module_t event_module,
                               int event_id, void *event_data)
{
    // 打印接收到的事件信息
    BK_EXAMPLE_LOG(TAG,"rx event <%d %d>\n", event_module, event_id);
    netif_event_got_ip4_t *got_ip;

    switch (event_id)
    {
        // 处理获取到 IPv4 地址的事件
        case EVENT_NETIF_GOT_IP4:
            got_ip = (netif_event_got_ip4_t *)event_data;
            netif_ip4_config_t ip4_info = {0};
            BK_EXAMPLE_LOG(TAG,"%s got ip:%s\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "unknown netif", got_ip->ip);

            BK_RETURN_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_STA, &ip4_info));
            BK_EXAMPLE_LOG(TAG, "ip=%s,gate=%s,mask=%s,dns=%s\r\n",
                           ip4_info.ip, ip4_info.gateway, ip4_info.mask, ip4_info.dns);

            if (g_wifi_cmd_sema != NULL)
            {
                rtos_set_semaphore(&g_wifi_cmd_sema);
            }
            break;

        default:
            BK_EXAMPLE_LOG(TAG,"rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}


// WiFi STA 模式事件回调函数，处理 WiFi STA 模式相关事件
static int wifi_sta_event_cb(void *arg, event_module_t event_module,
                             int event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;

    switch (event_id)
    {
        case EVENT_WIFI_STA_CONNECTED:
            sta_connected = (wifi_event_sta_connected_t *)event_data;
            BK_EXAMPLE_LOG(TAG, "STA connected to %s\n", sta_connected->ssid);
            break;

        case EVENT_WIFI_STA_DISCONNECTED:
            sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
            BK_EXAMPLE_LOG(TAG, "STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
            break;

        default:
            BK_EXAMPLE_LOG(TAG, "rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

// 初始化 WiFi STA 模式并尝试连接到指定的 WiFi 网络
static void wifi_sta_init_and_connect(void)
{
    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

    // 注册 WiFi 事件回调函数，处理所有 WiFi 相关事件
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_sta_event_cb, NULL));
    // 注册网络接口事件回调函数，处理所有网络接口相关事件
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL));

    //设置 WiFi SSID、密码
    strncpy(sta_config.ssid, CONFIG_EXAMPLE_WIFI_STA_SSID, WIFI_SSID_STR_LEN);
    strncpy(sta_config.password, CONFIG_EXAMPLE_WIFI_STA_PASSWORD, WIFI_PASSWORD_LEN);

    BK_EXAMPLE_LOG(TAG, "ssid:%s password:%s\n", sta_config.ssid, sta_config.password);
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    // 启动 WiFi STA 模式
    BK_LOG_ON_ERR(bk_wifi_sta_start());
}

// FTP 服务器线程函数
static void ftp_server_thread(void *arg)
{
    int err;

    #if CONFIG_VFS
    char *fs_name = NULL;
    #if FTP_VFS_DRIVER == DEV_LITTLEFS
    struct bk_little_fs_partition partition;
    BK_EXAMPLE_LOG(TAG,"=====mount lfs========\r\n");
    fs_name = "littlefs";
    partition.part_type = LFS_FLASH;
    bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);

    partition.part_flash.start_addr = pt->partition_start_addr;
    partition.part_flash.size = pt->partition_length;
    partition.mount_path = "/";

    // 挂载文件系统
    err = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
    BK_EXAMPLE_LOG(TAG,"mount ret=%d,addr:%x,size:%x\n", err,partition.part_flash.start_addr,partition.part_flash.size);
    #else
    struct bk_fatfs_partition partition;
    BK_EXAMPLE_LOG(TAG,"=====mount sdcard========\r\n");
	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_SDCARD;
	partition.mount_path = "/";

	err = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
    BK_EXAMPLE_LOG(TAG,"mount ret=%d,path:%s\n", err,partition.mount_path);
    #endif
    #endif

    // 初始化 WiFi STA 模式并尝试连接到指定的 WiFi 网络
    wifi_sta_init_and_connect();

    // 等待 WiFi 命令信号量，永久等待
    err = rtos_get_semaphore(&g_wifi_cmd_sema, BEKEN_WAIT_FOREVER);
    if (err != kNoErr)
    {
        goto _exit;
    }

    // 初始化 FTP 服务器
    BK_EXAMPLE_LOG(TAG, "Initializing FTP server...\n");
    if (ftp_init(FTP_THREAD_STACK_SIZE, FTP_THREAD_PRIORITY) != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG, "Failed to initialize FTP server!\n");
        goto _exit;
    }

    BK_EXAMPLE_LOG(TAG, "FTP server started successfully on port %d!\n", ftp_get_port());

_exit:
    rtos_deinit_semaphore(&g_wifi_cmd_sema);
    ftp_server_handle = NULL;
    rtos_delete_thread(NULL);
}

// 程序入口函数
int main(void)
{
    int err;

    // 系统初始化
    bk_init();
    // 设置 shell 日志级别为 INFO
    shell_set_log_level(BK_LOG_INFO);

    err = rtos_init_semaphore(&g_wifi_cmd_sema, 1);
    if (err != kNoErr)
    {
        BK_EXAMPLE_LOG(TAG, "create g_wifi_cmd_sema fail.\r\n");
        return -1;
    }

    // 创建 FTP 服务器线程
    err = rtos_create_thread(&ftp_server_handle, 5, "ftp_server",
                             (beken_thread_function_t)ftp_server_thread, 4096, 0);

    if (err != kNoErr)
    {
        BK_EXAMPLE_LOG(TAG, "create ftp_server thread fail.\r\n");
        return -1;
    }

    return 0;
}