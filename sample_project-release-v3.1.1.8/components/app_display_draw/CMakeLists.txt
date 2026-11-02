set(incs . sw dma2d)
set(srcs "")

if (CONFIG_APP_DISPLAY_DRAW_SOFTWARE)
	list(APPEND srcs
		app_display_draw.c
		sw/app_draw_sw.c
	)
endif()

if (CONFIG_APP_DISPLAY_DRAW_DMA2D)
	list(APPEND srcs
		app_display_draw.c
		dma2d/app_draw_hw_dma2d.c
	)
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common include bk_dma2d multimedia
)
