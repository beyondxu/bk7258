set(incs . web_config/inc web_config/example)

set(srcs
    ap_main.c
	web_config/example/apconfig_example.c
	web_config/src/webnet.c
	web_config/src/wn_mimetype.c
	web_config/src/wn_module.c
	web_config/src/wn_request.c
	web_config/src/wn_session.c
	web_config/src/wn_utils.c
	web_config/module/wn_module_asp.c
	web_config/module/wn_module_auth.c
	web_config/module/wn_module_cgi.c
	web_config/module/wn_module_index.c
	web_config/module/wn_module_alias.c
	web_config/module/wn_module_log.c
	web_config/module/wn_module_upload.c
	web_config/module/wn_module_ssi.c
	web_config/module/wn_module_dav.c
    )


set(priv_req lwip_intf_v2_1 json)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
