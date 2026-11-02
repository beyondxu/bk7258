set(incs .)
set(srcs ap_main.c dev_lcd.c boot_animation.c frame_buffer_app.c)

set(priv_req  media_service multimedia bk_network_provisioning bk_jpeg_decoder app_jpeg_hw_decode app_jpeg_file_animation)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
