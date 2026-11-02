set(incs)
set(srcs)

set(incs ./inc)

armino_component_register(
	INCLUDE_DIRS "${incs}"
	)
	
get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/libmoduleA.a NAME)
add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/libmoduleA.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})