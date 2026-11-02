set(incs .)
set(srcs ap_main.c dev_lcd.c lcd_st7701s_app.c frame_buffer_app.c)

set(priv_req  media_service multimedia bk_network_provisioning bk_jpeg_decoder app_jpeg_hw_decode)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
