set(BSP_PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/bsp_types.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/level.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/wad_file.h
)

set(BSP_PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/level.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/wad_file.cpp
)

add_library(bsp STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${BSP_PUBLIC_HEADERS}
	${BSP_PRIVATE_SOURCES}
)

appfw_module(bsp)
target_include_directories(bsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(bsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(bsp PUBLIC
	GLM_FORCE_SILENT_WARNINGS
)

target_link_libraries(bsp
	appfw
	glm::glm
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(bsp PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
)

source_group("Public Headers" FILES ${BSP_PUBLIC_HEADERS})
source_group("Private Headers" FILES ${BSP_PRIVATE_HEADERS})
source_group("Private Sources" FILES ${BSP_PRIVATE_SOURCES})
