set(incs .)
set(srcs "")

if (CONFIG_JPEG_DECODER_HW)
	list(APPEND srcs
		jpeg_hw_decode.c
	)
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common include bk_jpeg_decoder bk_vfs multimedia
)