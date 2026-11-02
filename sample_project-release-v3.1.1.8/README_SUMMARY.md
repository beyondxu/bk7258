# SMP示例仓库

## compile: 编译相关示例
- [第三方库如何编译以及使用](compile/compile_lib/README.md)
- [如何解决编译warning等级的报错](compile/ignore_error/README.md)

## components：示例工程的通用组件
- app_avi_player: avi播放组件
- app_avi_record: avi录制组件
- app_jpeg_file_animation: jpeg文件动画组件
- app_jpeg_hw_decode: jpeg硬解码组件

## tools：示例工程相关的工具
- [字库bin打包工具](tools/font_bin_merge/README.md)
- [Jpeg图片yuv 420转422格式工具](tools/jpgconvert/README.md)
- [littlefs linux下打包工具](tools/littlefs/README.md)
- [lvgl 图片解码.c如何保存为bin文件](tools/lvgl_C_file_to_image_bin/README.md)

## wifi：
- [Wi-Fi 扫描示例](wifi/scan/README.md)
- [Wi-Fi AP示例](wifi/softap/README.md)
- [Wi-Fi AP示例](wifi/softap_sta/README.md)
- [Wi-Fi STA示例](wifi/sta_connect/README.md)
- bridge: 开发中
- p2p_gc: 开发中
- p2p_go: 开发中
- provisioning
  - with_ble: 开发中
  - with_http_webserver: 开发中

## bluetooth：ble和BT相关示例
- gatt_client: 开发中
- gatt_server: 开发中
- headset: 开发中

## peripherals：外设驱动相关示例
- adc: 开发中
- audio
  - mic: 开发中
  - mic_speaker: 开发中
  - speaker: 开发中
- dma2d
  - [DMA2D 示例](peripherals/dma2d/memcpy/README.md)
- gpio
  - [GPIO 输入示例](peripherals/gpio/input/README.md)
  - [GPIO 低压休眠 io 保持示例](peripherals/gpio/keep_io_lowerpower/README.md)
  - [GPIO 软重启 io 保持示例](peripherals/gpio/keep_io_reboot/README.md)
  - [GPIO 输出示例](peripherals/gpio/output/README.md)
- i2c: 开发中
- jpeg
  - hardware_decode
    - [jpeg整帧硬解码输出RGB565大端格式示例](peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_big_endian/README.md)
    - [jpeg整帧硬解码输出RGB565小端格式示例](peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_little_endian/README.md)
    - [jpeg整帧硬解码输出yuv422格式示例](peripherals/jpeg/hardware_decode/decode_jpg_to_yuv422/README.md)
- lcd
  - mcu_8080: 开发中
  - qspi: 开发中
  - rgb565: 开发中
  - rgb666
    - [lcd RGB666屏显示jpeg图片](peripherals/lcd/rgb666/display_jpg/README.md)
  - rgb888: 开发中
  - spi: 开发中
- pwm: 开发中
- qspi: 开发中
- qspi_flash: 开发中
- spi: 开发中
- spi_flash: 开发中
- timer: 开发中
- uart: 开发中

## multimedia：多媒体相关示例
- audio_codec
  - mp3: 开发中
- avi
  - [MULTIMEDIA AVI播放示例](multimedia/avi/display_lcd_RGB565/README.md)
  - [MULTIMEDIA AVI录制示例](multimedia/avi/dvp_camera_recode_avi/README.md)
- [jpeg开机动画显示](multimedia/boot_anim/README.md)
- lvgl
  - V8.X
    - [lvgl V8.3.9 文字显示使用字库bin文件](multimedia/lvgl/V8.X/font_bin_display/README.md)
    - [lvgl V8.3.9 显示gif动画](multimedia/lvgl/V8.X/gif_display/README.md)
    - [lvgl V8.3.9 如何显示解码图片bin](multimedia/lvgl/V8.X/image_bin_display/README.md)
    - [lvgl V8.3.9 显示jpeg硬解码预先解的图片](multimedia/lvgl/V8.X/jpeg_pre_hw_decode_display/README.md)
    - [lvgl V8.3.9 显示jpeg软解码的图片](multimedia/lvgl/V8.X/jpeg_pre_sw_decode_display/README.md)
    - [lvgl V8.3.9 显示jpeg/png路径的图片](multimedia/lvgl/V8.X/picture_direct_display/README.md)
    - [lvgl V8.3.9 显示预解码的png图片](multimedia/lvgl/V8.X/png_pre_decode_display/README.md)
    - [lvgl V8.3.9 显示jpeg硬解码预先解的图片](multimedia/lvgl/V8.X/rotate_lcd_display/README.md)

## net_protocol：网络协议相关示例
- ali_mqtt: 开发中
- ftp
  - ftp_server: 开发中
- paho_mqtt: 开发中
- tcp_client: 开发中
- tcp_server: 开发中
- udp: 开发中
- webclient
  - [webclient 文件下载（整包连续）示例](net_protocol/webclient/download_file/README.md)
  - [webclient 断点续传下载文件示例](net_protocol/webclient/download_file_with_breakpoint/README.md)
  - [webclient 请求http短数据示例](net_protocol/webclient/http_get_shorter_data/README.md)
  - [webclient http post数据示例](net_protocol/webclient/http_post/README.md)
- websocket: 开发中

## rtc

## storage：存储相关示例
- docs: 开发中
- external_qspi_flash_with_littlefs: 开发中
- external_spi_flash_with_littlefs: 开发中
- internal_flash_fatfs: 开发中
- [芯片内部flash分区作为littlefs示例](storage/internal_flash_littlefs/README.md)
- internal_flash_operate: 开发中
- sdcard_with_fatfs: 开发中

**说明：**
- 带有超链接的项目表示已有完整的README文档
- 标记为开发中的项目表示该功能正在开发或尚未完成文档
- 点击超链接可以查看详细的使用说明和示例代码
