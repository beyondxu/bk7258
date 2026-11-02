set(incs include)
set(srcs)

list(APPEND incs
	inc
	)

if (CONFIG_FTP_SERVER)
list(APPEND srcs
	src/ftp_session_cmd.c
	src/ftp_session.c
	src/ftp.c
	)


endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_vfs bk_rtos lwip_intf_v2_1)

