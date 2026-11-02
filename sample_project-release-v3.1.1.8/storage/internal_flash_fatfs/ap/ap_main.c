#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include "bk_posix.h"
#include "driver/flash_partition.h"

#define TAG "VFS"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CONFIG_INIT_EMPTY_FS     0
#define VFS_MOUNT_PATH           "/root"
#define TEST_FILE_NAME           VFS_MOUNT_PATH"/1.txt"

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

static bk_err_t _format_fs(void)
{
    struct bk_fatfs_partition partition;
    char *fs_name = NULL;
    bk_err_t ret = BK_FAIL;

    bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
    uint32_t flash_start = fs_partition->partition_start_addr;
    uint32_t flash_size = fs_partition->partition_length;

    fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_FLASH;

    ret = mkfs("PART_NONE", fs_name, &partition);
    LOGI("[%s][%d] mkfs ret:%d, start_addr:%p, flash_size:%p\r\n", __FUNCTION__, __LINE__, ret, flash_start, flash_size);

    return ret;
}

//挂载文件系统
static bk_err_t _mount_fs(char *mount_path)
{
    struct bk_fatfs_partition partition;
    char *fs_name = NULL;
    bk_err_t ret = BK_FAIL;

    bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
    uint32_t flash_start = fs_partition->partition_start_addr;
    uint32_t flash_size = fs_partition->partition_length;

    fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_FLASH;
    partition.mount_path = mount_path;

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

    LOGI("[%s][%d] mount ret:%d, start_addr:%p, flash_size:%p\r\n", __FUNCTION__, __LINE__, ret, flash_start, flash_size);
    
    return ret;
}

static bk_err_t _empty_fs_handle(void)
{
    bk_err_t ret = BK_FAIL;

    do{
        if(BK_OK != _mount_fs(VFS_MOUNT_PATH))
        {
            if(BK_OK != _format_fs())
            {
                LOGE("[%s][%d] format fail\r\n", __FUNCTION__, __LINE__);
                TEST_FAIL();
                //BK_ASSERT(false);
                break;
            }

            if(BK_OK != _mount_fs(VFS_MOUNT_PATH))
            {
                LOGE("[%s][%d] mount fail\r\n", __FUNCTION__, __LINE__);
                TEST_FAIL();
                //BK_ASSERT(false);
                break;
            }

            ret = BK_OK;
        }
    }while(0);

    if(BK_OK == ret)
    {
        LOGE("[%s][%d] mount success\r\n", __FUNCTION__, __LINE__);
    }

    return ret;
}

//获取文件长度
static int _get_filesize(char *filename)
{
    int fd = 0;
    int file_size = 0;

    do{
        fd = open(filename, O_RDONLY);
        if(fd < 0)
            break;

        lseek(fd, 0, SEEK_END);
        file_size = ftell(fd);
        close(fd);
    }while(0);
    
    return file_size;
}

static void _read_dir(char *file_path)
{
    char path[128] = {0};
    DIR *dir = NULL;
    char filename[128] = {0};
    uint32_t total_size = 0, count_index = 0;
    static int flag = 0;

    os_snprintf(path, sizeof(path), "%s", file_path);
    dir = opendir(path);
    if(!dir)
    {
        os_printf("[%s][%d] opendir fail:%s\r\n", __FUNCTION__, __LINE__, path);
        return ;
    }

    struct dirent *entry;

    if(0 == flag)
    {
        LOGI("==============================================================================================================\r\n");
        LOGI("|| [%-12s] %-60s %16s             || \r\n", "path", "filename", "filesize");

        flag = 1;
    }
    
    while((entry = readdir(dir)) != NULL)
    {
        if(DT_REG == entry->d_type)
        {
            os_snprintf(filename, sizeof(filename), "%s/%s", file_path, entry->d_name);
            int size = _get_filesize(filename);
            LOGI("|| [%-12s] %-64s %6d Byte    ||\r\n", file_path, filename, size);
            total_size += _get_filesize(filename);
            count_index ++;
        }
        else if(DT_DIR == entry->d_type)
        {
            if(0 != os_strcmp(entry->d_name, ".") && 0 != os_strcmp(entry->d_name, ".."))
            {
                if(0 == os_strcmp(file_path, "/") || 0 == os_strcmp(file_path, "//"))
                    os_snprintf(path, sizeof(path), "%s%s", file_path, entry->d_name);
                else
                    os_snprintf(path, sizeof(path), "%s/%s", file_path, entry->d_name);
                //bk_printf("[%s]\r\n", path);
                _read_dir(path);
            }
        }
    }

    if(count_index > 0)
    {
        LOGI("========================================the sector total size = %d KB num = %d===============================\r\n",(total_size/1024),count_index);
    }
    if(dir)
    {
        closedir(dir);
    }
}

//文件写测试
static bk_err_t _file_write_test(void)
{
    int fd = -1;
    bk_err_t ret = BK_FAIL;

    do{
        fd = open(TEST_FILE_NAME, O_RDWR | O_CREAT | O_APPEND);
        if (fd < 0) {
            LOGE("[%s][%d] open %s fail\n", __FUNCTION__, __LINE__, TEST_FILE_NAME);
            TEST_FAIL();
            break;
        }

        write(fd, "helloworld, this is beken vfs test\r\n", strlen("helloworld, this is beken vfs test\r\n")+1);
        close(fd);
        ret = BK_OK;

        LOGI("[%s][%d] write file success\r\n", __FUNCTION__, __LINE__);
    }while(0);

    return ret;
}

//文件读测试
static bk_err_t _file_read_test(void)
{
    int fd = -1;
    bk_err_t ret = BK_FAIL;
    char *buff = NULL;
    uint32_t filesize = 0;

    do{
        filesize = _get_filesize(TEST_FILE_NAME);
        LOGI("[%s][%d] %s filesize:%d\n", __FUNCTION__, __LINE__, TEST_FILE_NAME, filesize);

        buff = os_malloc(filesize+1);
        if(!buff)
        {
            LOGE("[%s][%d] malloc fail\n", __FUNCTION__, __LINE__);
            break;
        }

        os_memset(buff, 0, filesize+1);
        fd = open(TEST_FILE_NAME, O_RDONLY);
        if (fd < 0) {
            LOGE("[%s][%d] open %s fail\n", __FUNCTION__, __LINE__, TEST_FILE_NAME);
            TEST_FAIL();
            break;
        }

        read(fd, buff, filesize);
        close(fd);
        ret = BK_OK;
        TEST_PASS();

        LOGI("[%s][%d] read:%s\r\n", __FUNCTION__, __LINE__, buff);
    }while(0);

    if(buff)
    {
        os_free(buff);
    }

    return ret;
}

//文件删除测试
static bk_err_t _file_del_test(void)
{
    return unlink(TEST_FILE_NAME);
}

int main(void)
{
    bk_init();
    media_service_init();

    //文件系统初始化
#if CONFIG_INIT_EMPTY_FS
    if(BK_OK != _empty_fs_handle())
    {
        LOGE("[%s][%d] fs init fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
    }
#else
    if(BK_OK != _mount_fs(VFS_MOUNT_PATH))
    {
        LOGE("[%s][%d] fs mount fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
    }
#endif

    //遍历文件系统所有文件
    _read_dir(VFS_MOUNT_PATH);

	//基于flash分区的fatfs不支持写操作
    //_file_write_test();

	//基于flash分区的fatfs仅支持读操作
    _file_read_test();

    return 0;
}
