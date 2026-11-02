set(incs . )

set(srcs
    ap_main.c
	paho_mqtt_demo.c
    )


set(priv_req lwip_intf_v2_1 json paho-mqtt psa_mbedtls)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
