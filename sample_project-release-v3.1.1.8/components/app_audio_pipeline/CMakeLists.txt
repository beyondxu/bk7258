set(incs .)
set(srcs bk_audio_pipeline.c)

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES media_service multimedia wanson_armino
)