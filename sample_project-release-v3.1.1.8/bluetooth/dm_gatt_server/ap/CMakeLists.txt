set(incs .)
set(srcs 
    ap_main.c
)

if (CONFIG_BT)
	list(APPEND incs
		app_bluetooth/storage
		app_bluetooth/gatt
		app_bluetooth/gatt/printer
		app_bluetooth
	)

	list(APPEND srcs
		app_bluetooth/storage/bluetooth_storage.c
		app_bluetooth/gatt/dm_gap_utils.c
		app_bluetooth/gatt/dm_gatt.c
		app_bluetooth/gatt/dm_gattc.c
		app_bluetooth/gatt/dm_gatts.c
		app_bluetooth/gatt/dm_gatt_cli.c
		app_bluetooth/gatt/dm_gatt_connection.c
		app_bluetooth/gatt/f618/f618_server.c
		app_bluetooth/gatt/fa00/fa00_server.c
		app_bluetooth/gatt/hogp/hogpd.c
		app_bluetooth/gatt/battery/battery_server.c
		app_bluetooth/gatt/dis/dis.c
		app_bluetooth/gatt/printer/ble_printer_service.c
		app_bluetooth/gatt/printer/printer_protocol.c
        app_bluetooth/gatt/printer/printer_mapper.c
        app_bluetooth/gatt/printer/printer_page.c
               app_bluetooth/gatt/printer/hw_printer_stub.c
               app_bluetooth/gatt/printer/motor_sdk_driver.c
                app_bluetooth/gatt/printer/tph_driver.c

		)
endif()


set(priv_req  media_service multimedia bk_network_provisioning lwip_intf_v2_1)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
