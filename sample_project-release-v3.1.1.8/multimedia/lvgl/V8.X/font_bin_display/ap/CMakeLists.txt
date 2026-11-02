set(incs . ./font)
set(srcs 
		ap_main.c 
		dev_lcd.c 
		lvgl_app_ui.c
		ui_screen1.c
		font/app_font.c
		font/simhei_font16.c
		font/simhei_font24.c
		)

set(priv_req  media_service multimedia bk_network_provisioning lvgl)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
