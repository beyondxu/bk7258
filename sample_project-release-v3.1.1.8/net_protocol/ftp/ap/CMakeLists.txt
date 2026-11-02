set(incs . )

set(srcs
    ap_main.c
    )


set(priv_req lwip_intf_v2_1 json ftp psa_mbedtls bk_vfs)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
