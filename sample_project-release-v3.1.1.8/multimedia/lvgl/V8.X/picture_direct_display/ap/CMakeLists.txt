set(incs .)
set(srcs 
		ap_main.c 
		dev_lcd.c 
		lvgl_app_ui.c
		ui_screen1.c
		)

set(priv_req  media_service multimedia bk_network_provisioning bk_jpeg_decoder lvgl)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
