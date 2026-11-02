set(incs .
         ./inc
         ./components/bk_avi/
    )

set(srcs
    ap_main.c
    )

list(APPEND srcs
    ./src/dev_lcd.c
    ./src/dev_camera.c
    ./src/dev_audio.c
    ./src/fatfs_fs.c
    ./src/video_frame_queue.c
    ./src/audio_frame_queue.c
    ./src/cli_cmd.c
)

set(priv_req  media_service multimedia avdk_utils bk_camera app_avi_record)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -O3)


