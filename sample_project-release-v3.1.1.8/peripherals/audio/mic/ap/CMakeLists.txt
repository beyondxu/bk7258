set(incs .)
set(srcs ap_main.c)

set(priv_req media_service multimedia audio_pipeline raw_stream onboard_speaker_stream onboard_mic_stream)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
