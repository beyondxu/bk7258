set(incs .)
set(srcs ap_main.c)

list(APPEND srcs
	websocket_client.c
	wifi_manager.c
)

set(priv_req  media_service multimedia bk_network_provisioning lwip_intf_v2_1 bk_websocket psa_mbedtls)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
