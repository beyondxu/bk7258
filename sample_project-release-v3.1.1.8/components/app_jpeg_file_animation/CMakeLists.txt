set(incs .)
set(srcs "")

list(APPEND srcs
	jpeg_file_animation.c
)

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common include app_jpeg_hw_decode multimedia
)