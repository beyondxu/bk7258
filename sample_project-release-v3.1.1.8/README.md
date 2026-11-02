# SMP示例仓库

[TOC]  

<br><br>
**说明：**
- 带有超链接的项目表示已有完整的README文档
- 标记为开发中的项目表示该功能正在开发或尚未完成文档
- 点击超链接可以查看详细的使用说明和示例代码

## compile: 编译相关示例
- [第三方库如何编译以及使用](compile/compile_lib/README.md)
- [如何解决编译warning等级的报错](compile/ignore_error/README.md)

## asr：ASR相关示例
- [ASR KWS 关键词识别 示例](asr/kws_demo/README.md)

## components：示例工程的通用组件

> **注意** ：[如何使用组件，请点击这里](components/README.md)<br>

- [app_avi_player](components/app_avi_player/README.md)
- [AVI 文件录制组件](components/app_avi_record/README.md)
- [显示绘制组件（颜色填充、图片数据搬移、透明度图片数据融合、文字融合）](components/app_display_draw/README.md)
- [JPEG 图片动画播放组件](components/app_jpeg_file_animation/README.md)
- [jpeg图片硬解码 组件](components/app_jpeg_hw_decode/README.md)
- [dma2d 颜色填充、拷贝、融合组件](components/app_dma2d_hw_draw/README.md)
- [app_asr 组件](components/bk_app_asr/README.md)
- [kws 组件](components/bk_kws/README.md)

## tools：示例工程相关的工具
- [字库bin打包工具](tools/font_bin_merge/README.md)
- [Jpeg图片yuv 420转422格式工具](tools/jpgconvert/README.md)
- [littlefs linux下打包工具](tools/littlefs/README.md)
- [lvgl 图片解码.c如何保存为bin文件](tools/lvgl_C_file_to_image_bin/README.md)
- [ffmpeg图片预解码工具](tools/ffmpeg_bat/README.md)

## wifi：
- [Wi-Fi 扫描示例](wifi/scan/README.md)
- [Wi-Fi AP示例](wifi/softap/README.md)
- [Wi-Fi AP示例](wifi/softap_sta/README.md)
- [Wi-Fi STA示例](wifi/sta_connect/README.md)
- [Wi-Fi P2P GC示例](wifi/p2p_gc/README.md)
- [Wi-Fi P2P GO示例](wifi/p2p_go/README.md)
- provisioning
  - [BLE WiFi配网示例](wifi/provisioning/with_ble/README.md)
  - [HTTP WebServer WiFi配网示例](wifi/provisioning/with_http_webserver/README.md)

## bluetooth：ble和BT相关示例
- gatt_client: 开发中
- [BLE GATTS示例，BLE数据传输](bluetooth/gatt_server/README.md)
- [DM GATTS示例，BLE数据传输和HID功能](bluetooth/dm_gatt_server/README.md)
- [DM GATTC示例](bluetooth/dm_gatt_client/README.md)
- headset: 开发中

## peripherals：外设驱动相关示例
- [adc: ADC SINGLE MODE 示例](peripherals/adc/single_mode/README.md)
- audio
  - [mic示例](peripherals/audio/mic/README.md)
  - [mic_speaker示例](peripherals/audio/mic_speaker/README.md)
  - [speaker示例](peripherals/audio/speaker/README.md)
- dma2d
  - [DMA2D图像融合示例（ARGB8888前景融合到RGB565背景）](peripherals/dma2d/fg_argb888_blend_bg_rgb565/README.md)
  - [DMA2D颜色填充示例](peripherals/dma2d/fill_color/README.md)
  - [DMA2D内存拷贝示例（RGB565前景拷贝到RGB565背景）](peripherals/dma2d/mem_copy/README.md)
- gpio
  - [GPIO 输入示例](peripherals/gpio/input/README.md)
  - [GPIO 低压休眠 io 保持示例](peripherals/gpio/keep_io_lowerpower/README.md)
  - [GPIO 软重启 io 保持示例](peripherals/gpio/keep_io_reboot/README.md)
  - [GPIO 输出示例](peripherals/gpio/output/README.md)
- i2c
  - [硬件I2C master示例](peripherals/i2c/master_mode/README.md)
  - [硬件I2C slave示例](peripherals/i2c/slave_mode/README.md)
- jpeg
  - hardware_decode
    - [jpeg整帧硬解码输出RGB565大端格式示例](peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_big_endian/README.md)
    - [jpeg整帧硬解码输出RGB565小端格式示例](peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_little_endian/README.md)
    - [jpeg整帧硬解码输出yuv422格式示例](peripherals/jpeg/hardware_decode/decode_jpg_to_yuv422/README.md)
- [mcu_8080: lcd mcu-8080屏显示jpeg图片](peripherals/lcd/mcu_8080/display_jpg/README.md)
- [qspi: lcd qspi屏显示jpeg图片](peripherals/lcd/qspi/display_jpg/README.md)
- [rgb565: lcd RGB565屏显示jpeg图片](peripherals/lcd/rgb565/display_jpg/README.md)
- [rgb666: lcd RGB666屏显示jpeg图片](peripherals/lcd/rgb666/display_jpg/README.md)
- [rgb888: lcd RGB666屏显示jpeg图片](peripherals/lcd/rgb888/display_jpg/README.md)
- [spi: lcd spi屏显示jpeg图片](peripherals/lcd/spi/display_jpg/README.md)
- pwm
  - [PWM 捕获模式示例](peripherals/pwm/capture_mode/README.md)
  - [PWM 周期输出示例](peripherals/pwm/period_output/README.md)
- qspi: 开发中
- [qspi_flash: QSPI Flash（标准模式） 测试示例](peripherals/qspi_flash/standard/README.md)
- spi
  - [模拟 SPI 输出示例](peripherals/spi/sim_spi/README.md)
  - [硬件SPI master示例](peripherals/spi/spi_master/README.md)
  - [硬件SPI slave示例](peripherals/spi/spi_slave/README.md)
- [spi_flash: SPI Flash 测试示例](peripherals/spi_flash/standard/README.md)
- [timer: Timer 硬件定时器示例](peripherals/timer/README.md)
- uart
  - [串口收发数据使用示例](peripherals/uart/general_use/README.md)
  - [日志串口如何换为其它串口](peripherals/uart/log_uart_change/README.md)
  - [串口硬件流控使用示例](peripherals/uart/use_with_hw_flow_control/README.md)
  - [串口软件流控使用示例](peripherals/uart/use_with_sw_flow_control/README.md)
- wdt: 开发中

## multimedia：多媒体相关示例
- [audio_codec: mp3播放示例](multimedia/audio_codec/mp3/README.md)
- avi
  - [MULTIMEDIA AVI播放示例](multimedia/avi/display_lcd_RGB565/README.md)
  - [MULTIMEDIA AVI录制示例](multimedia/avi/dvp_camera_recode_avi/README.md)
- [boot_anim: jpeg开机动画显示](multimedia/boot_anim/README.md)
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
- [ftp: FTP服务器示例](net_protocol/ftp/README.md)
- [paho_mqtt: Paho MQTT示例](net_protocol/paho_mqtt/README.md)
- [rtc: RTC时间获取示例](net_protocol/rtc/README.md)
- [tcp_client: TCP客户端示例](net_protocol/tcp_client/README.md)
- [tcp_server: TCP服务器示例](net_protocol/tcp_server/README.md)
- [udp_client: UDP客户端示例](net_protocol/udp_client/README.md)
- [udp_server: UDP服务器示例](net_protocol/udp_server/README.md)
- webclient
  - [webclient 文件下载（整包连续）示例](net_protocol/webclient/download_file/README.md)
  - [webclient 断点续传下载文件示例](net_protocol/webclient/download_file_with_breakpoint/README.md)
  - [webclient 请求http短数据示例](net_protocol/webclient/http_get_shorter_data/README.md)
  - [webclient http post数据示例](net_protocol/webclient/http_post/README.md)
- [websocket: WebSocket客户端示例](net_protocol/websocket/README.md)

## storage：存储相关示例
- docs: 开发中
- [external_qspi_flash_with_littlefs: 外部QSPI Flash作为littlefs示例](storage/external_qspi_flash_with_littlefs/README.md)
- [external_spi_flash_with_littlefs: 外部SPI Flash作为littlefs示例](storage/external_spi_flash_with_littlefs/README.md)
- [internal_flash_fatfs: 芯片内部flash分区作为fatfs示例](storage/internal_flash_fatfs/README.md)
- [internal_flash_littlefs: 芯片内部flash分区作为littlefs示例](storage/internal_flash_littlefs/README.md)
- [internal_flash_operate: 芯片内部flash分区读写示例](storage/internal_flash_operate/README.md)
- [sdcard_with_fatfs: 挂载SD卡作为fatfs示例](storage/sdcard_with_fatfs/README.md)
