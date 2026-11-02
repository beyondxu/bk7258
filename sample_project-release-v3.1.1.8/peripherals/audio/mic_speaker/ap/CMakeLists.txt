set(incs .)
set(srcs ap_main.c)

set(priv_req media_service multimedia wanson_armino app_audio_pipeline)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
