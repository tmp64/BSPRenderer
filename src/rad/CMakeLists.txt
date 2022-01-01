set(SOURCE_FILES
	CMakeLists.txt
	include/rad/level_config.h
	include/rad/rad_config.h
	include/rad/rad_sim.h

	src/anorms.h
	src/bouncer.cpp
	src/bouncer.h
	src/coords.h
	src/filters.h
	src/level_config.cpp
	src/lightmap_writer.cpp
	src/lightmap_writer.h
	src/patch_divider.cpp
	src/patch_divider.h
	src/patch_list.cpp
	src/patch_list.h
	src/rad_config.cpp
	src/rad_sim.cpp
	src/rad_sim_impl.cpp
	src/rad_sim_impl.h
	src/sparse_vismat.cpp
	src/sparse_vismat.h
	src/surface.cpp
	src/types.h
	src/vflist.cpp
	src/vflist.h
	src/vismat.cpp
	src/vismat.h
)

add_library(rad STATIC
	${SOURCE_FILES}
)

appfw_module(rad)
target_include_directories(rad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(rad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(rad
	appfw
	app_base
	bsp
	taskflow
	yaml-cpp
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(rad PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
	[["rad_sim_impl.h"]]
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
