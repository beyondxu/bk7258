set(incs .)
set(srcs "")

if (CONFIG_MEDIA_DAM2D)
	list(APPEND srcs
		app_dma2d_hw_draw.c
	)
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common include 
)