set(incs .)
set(srcs 
    ap_main.c
    kws_demo.c
    )

set(priv_req bk_cli media_service multimedia avdk_utils bk_app_asr bk_voice_service bk_asr_service)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
