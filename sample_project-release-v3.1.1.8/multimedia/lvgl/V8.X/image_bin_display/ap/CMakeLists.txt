set(incs .)
set(srcs 
		ap_main.c 
		dev_lcd.c 
		lvgl_app_ui.c
		ui_resource.c
		ui_screen1.c
		)

set(priv_req  media_service multimedia bk_network_provisioning lvgl)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
