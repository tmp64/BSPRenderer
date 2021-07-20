set(PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/bouncer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/level_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/lightmap_writer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/patch_list.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/patch_tree.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/rad_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/rad_sim.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/sparse_vismat.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/types.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/vflist.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/rad/vismat.h

)

set(PRIVATE_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/src/plat.h
)

set(PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/bouncer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/level_config.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/lightmap_writer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/patch_list.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/patch_tree.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/plat_win.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/rad_config.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/rad_sim.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/sparse_vismat.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vflist.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vismat.cpp
)

add_library(rad STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${PUBLIC_HEADERS}
	${PRIVATE_HEADERS}
	${PRIVATE_SOURCES}
)

appfw_module(rad)
target_include_directories(rad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(rad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(rad
	appfw
	app_base
	bsp
	nlohmann_json
	taskflow
	yaml-cpp
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(rad PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
	<rad/rad_sim.h>
)

source_group("Public Headers" FILES ${PUBLIC_HEADERS})
source_group("Private Headers" FILES ${PRIVATE_HEADERS})
source_group("Private Sources" FILES ${PRIVATE_SOURCES})
