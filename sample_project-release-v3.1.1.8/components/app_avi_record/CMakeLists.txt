set(incs)
set(srcs)

set(incs .)
set(srcs ./bk_avi_record.c)

armino_component_register(
	INCLUDE_DIRS "${incs}"
	SRCS "${srcs}"
	PRIV_REQUIRES bk_common include multimedia
	)
	
get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/bk_libavi.a NAME)
add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk_libavi.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
