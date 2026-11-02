set(incs .)
set(srcs ap_main.c)

list(APPEND srcs
	wifi_manager.c
)

set(priv_req  media_service multimedia bk_network_provisioning lwip_intf_v2_1 app)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
