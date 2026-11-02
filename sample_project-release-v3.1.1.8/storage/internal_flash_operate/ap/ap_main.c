#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>

#include "driver/flash_partition.h"

#define TAG "FLASH"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

//扇区大小为4K Bytes
#define FLASH_SECTOR_SIZE	0x1000

//读写的目标分区
#define FLASH_PARTITION BK_PARTITION_USR_CONFIG

//FLASH写测试
static bk_err_t _flash_write_test(void)
{
	bk_err_t ret = BK_FAIL;

	uint8_t *buffer;
	int len = 100;
	uint32_t offset = 2 * FLASH_SECTOR_SIZE;	//操作第三个扇区
	int i;

	do 
	{
		buffer = os_malloc(len);
		if (!buffer) 
		{
			LOGE("[%s][%d] malloc fail\n", __FUNCTION__, __LINE__);
			break;
		}
		for (i = 0; i <len; i++) 
		{
			buffer[i] = (uint8_t)i;
		}

		ret = bk_flash_partition_erase(FLASH_PARTITION, offset, FLASH_SECTOR_SIZE);	//在写flash前需要先擦除一下该扇区
		if (ret) 
		{		
			LOGE("[%s][%d] erase fail\n", __FUNCTION__, __LINE__);
			TEST_FAIL();
			break;
		}

		ret = bk_flash_partition_write(FLASH_PARTITION, buffer, offset, len);
		if (ret) 
		{		
			LOGE("[%s][%d] write fail\n", __FUNCTION__, __LINE__);
			TEST_FAIL();
			break;
		}
		
        ret = BK_OK;
        TEST_PASS();
	} while(0);

	if (buffer)
		os_free(buffer);

	return ret;
}

//FLASH读测试
static bk_err_t _flash_read_test(void)
{
	bk_err_t ret = BK_FAIL;

	uint8_t *buffer;
	int len = 100;
	uint32_t offset = 2 * FLASH_SECTOR_SIZE;	//操作第三个扇区
	int i;

	do 
	{
		buffer = os_malloc(len);
		if (!buffer) 
		{
			LOGE("[%s][%d] malloc fail\n", __FUNCTION__, __LINE__);
			break;
		}
		memset(buffer, 0, len);

		ret = bk_flash_partition_read(FLASH_PARTITION, buffer, offset, len);
		if (ret) 
		{		
			LOGE("[%s][%d] write fail\n", __FUNCTION__, __LINE__);
			TEST_FAIL();
			break;
		}

		for (i = 0; i <len; i++) //检查该扇区的内容, 参见_flash_write_test()所写内容
		{
			if (buffer[i] != (uint8_t)i) 
			{
				LOGE("[%s][%d] read fail at pos %d, expect value is %d, but actual value is %d\n",
						__FUNCTION__, __LINE__, i, i, buffer[i]);
				TEST_FAIL();
				goto out;
			}
		}
		
        ret = BK_OK;
        TEST_PASS();
	} while(0);

out:
	if (buffer)
		os_free(buffer);

	return ret;
}

int main(void)
{
    bk_init();
    media_service_init();

    if (BK_OK != _flash_write_test()) 
    {
    	return -1;
    }

    if (BK_OK != _flash_read_test()) 
    {
    	return -2;
    }

	return 0;
}
