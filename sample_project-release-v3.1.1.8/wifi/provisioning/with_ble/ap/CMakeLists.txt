set(incs .)
set(srcs ap_main.c ble_cfg_utils.c motor_sdk_driver.c tph_driver.c) 

set(priv_req   json)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
