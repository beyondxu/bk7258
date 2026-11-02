set(incs .)
set(srcs ap_main.c dev_lcd.c frame_buffer_app.c)

set(priv_req  media_service multimedia bk_network_provisioning app_display_draw)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
