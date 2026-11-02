#include "stdio.h"
#include "common.h"

extern const lv_img_dsc_t icon;   //图片数组说明

int main(void)
{
	FILE *fp = fopen("ram_image.bin", "wb");
	
	fwrite(icon.data, icon.data_size, 1, fp);
	
	fclose(fp);
	
	return 0;
}