# lvgl V8.3.9 文字显示使用字库bin文件

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关章节
 - [如何生成字库bin](https://blog.csdn.net/pingxiaozhao/article/details/115214586)
 - [字库bin打包工具](/tools/font_bin_merge/README.md)
 - [lcd如何显示jpeg图片](/peripherals/lcd/rgb666/display_jpg/README.md)
 
## 概述

lvgl字库显示，在字库不多时，直接是以.c文件随程序一起编译到bin文件里，但在显示的字比较多或者字体大小有多种时，编译的程序bin很大，导致本地烧录和ota时间比较长。这种场景可以将字库生成bin文件，让代码访问字库bin进行显示。

**字库bin放fatfs或者littlefs文件系统时，因涉及文件系统访问，显示文字的帧率比字库.c文件的方式低很多，所以字库bin需要裸放到flash里**。

使用字库bin的开发步骤如下：
1. 参考 **[如何生成字库bin](https://blog.csdn.net/pingxiaozhao/article/details/115214586)**，利用页面里的工具，转换得到字库bin，以及对应字库访问的.c文件；
2. 将对应的字库.c文件放到工程里，修改里面的static uint8_t *__user_font_getdata(int offset, int size)函数，实现从flash 访问字库bin（可参考示例代码simhei_font16.c或simhei_font24.c）；
3. 利用 **[字库bin打包工具](/tools/font_bin_merge/README.md)**，将多个字库bin打包到一个bin文件里；
4. 将打包后的字库bin文件，烧录到第二步.c代码访问的flash地址；

<br>
> **备注** ：目前示例显示文字速度慢，是内置flash访问速度慢导致，sdk后续版本会优化；

本示例是利用lvgl V8.3.9版本，将字体大小16和24的字库bin，合为一个bin，烧录到flash，实现ui的显示，包含以下功能：
1. 新建字库分区，用于存储字库bin文件；
2. 如何打开lcd；
3. lvgl初始化；
4. lvgl显示字库bin里的字体大小16和24的文字；

## 硬件环境

- 核心板：BK7258_QFN88_V3
- 显示屏：st7701s 480X480 RGB屏幕

> **备注** ：如果更换为其它屏幕，需修改dev_lcd.c中的屏幕参数: rgb_config里的屏的lcd_device，相关io管脚：ldo、背光、复位、cs、clk、sda；

![bk_vfs](/multimedia/boot_anim/v3_demo_board.jpg)

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_LCD | lcd 使能配置，需要配置为y |
| CONFIG_LCD_ST7701S | st7701s屏驱使能配置，配置为y，换其它屏配置为n |
| CONFIG_LVGL | lvgl使能配置，配置为y |
| CONFIG_LVGL_V8 | lvgl V8版本使能配置，配置为y |
| CONFIG_LV_COLOR_DEPTH | lvgl 颜色深度，依据屏驱输出fmt配置，如屏驱输出fmt为RGB565，则配置为16 |
| CONFIG_LVGL_FRAME_BUFFER_NUM | 刷屏的frame buffer数量，默认2个，用于解决lvgl RGB和MCU屏切屏问题，有内存的SPI或QSPI屏可以配置为1|
| CONFIG_LV_FRAME_DMA2D_COPY | DMA2D拷贝使能配置，刷屏的frame buffer数量为2时，用于两个buffer同步的拷贝方式，默认配置为y |
| CONFIG_DMA2D | CONFIG_LV_FRAME_DMA2D_COPY有开启 ，需要配置为y |

## 字库分区

新增字库分区修改工程font_bin_display/partition/bk7258/auto_partition.csv文件，如下所示，起始地址会在程序编译完成后显示。
``` c
    # The bin size whose type is code occupies more space in flash,,,,,
    # The occupied space is: bin size*34/32(and 68K aligned),,,,,
    # easyflash:beken CP some components use it
    # easyflash_ap:beken AP some components use it
    #Name,Offset,Size,Type,Read,Write
    primary_bootloader,,68k,code,TRUE,FALSE
    primary_cp_app,,1360k,code,TRUE,FALSE
    primary_ap_app,,1156k,code,TRUE,FALSE
    ota,,1360K,data,TRUE,TRUE
    usr_config,,1024K,data,TRUE,TRUE
    usr_font,,1024K,data,TRUE,TRUE                           //新增字库分区
    easyflash,0x7fa000,8K,data,TRUE,TRUE
    easyflash_ap,0x7fc000,8K,data,TRUE,TRUE
    sys_rf,0x7fe000,4k,data,TRUE,TRUE
    sys_net,0x7ff000,4k,data,TRUE,TRUE
``` 
## 相关接口
``` c
    /**
     * @brief 根据字体名称获取字体在Flash中的起始地址
     * 
     * 该函数用于获取指定字体在Flash分区中的实际起始地址，以便进行字体数据的读取和使用。
     * 字体数据存储在BK_PARTITION_USR_FONT分区中，函数会根据字体类型返回对应的偏移地址。
     * 
     * @param font_name 字体名称枚举值
     *        - FONT_SIMHEI_16: 16号黑体字
     *        - FONT_SIMHEI_24: 24号黑体字
     * 
     * @return 字体在Flash中的起始地址
     *         - 成功：返回实际的Flash起始地址
     *         - 失败：返回0
     */
    uint32_t app_get_font_startaddr(FONT_NAME_E font_name)
    
    /**
     * @brief 根据字体名称获取LVGL字体对象
     * 
     * 该函数用于获取指定字体的lv_font_t类型对象，供LVGL图形库使用。
     * 
     * @param font_name 字体名称枚举值
     *        - FONT_SIMHEI_16: 16号黑体字
     *        - FONT_SIMHEI_24: 24号黑体字
     * 
     * @return LVGL字体对象指针
     *         - 成功：返回对应的lv_font_t对象指针
     *         - 失败：返回NULL
     */
    lv_font_t* app_get_font(FONT_NAME_E font_name)
```

## 相关代码
``` c
    //字库24字体访问flash，获取字库数据
    static uint8_t *__user_font_getdata(int offset, int size){
        uint32 startAddr = app_get_font_startaddr(FONT_SIMHEI_24);   //获取字库在flash的开始地址

        if(!g_simhei_font24_buffer)
        {
            g_simhei_font24_buffer = (uint8_t *)psram_malloc(FONT_BUFF_SIZE);
        }

        if(!g_simhei_font24_buffer)
        {
            FONT_LOGE("[%s][%d] malloc fail\r\n", __FUNCTION__, __LINE__);
            return NULL;
        }

        if(size > FONT_BUFF_SIZE)
        {
            FONT_LOGE("[%s][%d] size over buffer:%d>%d\r\n", __FUNCTION__, __LINE__, size, FONT_BUFF_SIZE);
            return NULL;
        }

        bk_flash_read_bytes(startAddr + offset, g_simhei_font24_buffer, size);

        return g_simhei_font24_buffer;
    }

    lv_obj_t *ui_label = lv_label_create(panel);
    lv_label_set_text(ui_label, "博通集成");
    lv_obj_set_width(ui_label, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_label, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_label, LV_ALIGN_TOP_MID);
    lv_obj_set_style_text_color(ui_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_label, app_get_font(FONT_SIMHEI_24), LV_PART_MAIN | LV_STATE_DEFAULT);    //获取字体
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/multimedia/lvgl/V8.X/font_bin_display
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下的all-app.bin；
- sample_project/tools/font_bin_merge/font-merge_v0.1.bin烧录到新建的字库usr_font分区，即0x004da000地址；
- 上电后，lcd屏上博通集成相关的文字介绍；

以下是上电运行后的关键串口log 输出：

```
[2025-12-17 17:36:57.652]  ap0:LVGL_DIS:I(1016):LVGL addr1:28023670, addr2:0, pixel size:23040, fb1:6048f7bc, fb2:6041ef70
[2025-12-17 17:36:57.652]  ap0:lvgl:I(1020):[lvgl_app_init][41] init success          //lvgl初始化成功
[2025-12-17 17:36:57.652]  ap0:font:I(1030):[app_get_fontbin_info][21] version:0.1   //字库版本0.1
[2025-12-17 17:36:57.776]  ap1:lvgl:I(1231):[dev_lcd_backlight_ctrl][80] on          //开启背光
```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
