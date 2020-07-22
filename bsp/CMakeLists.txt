set(BSP_PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/bsp_types.h
)

set(BSP_PRIVATE_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/src/this_module_info.h
)

set(BSP_PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	
	#${CMAKE_CURRENT_SOURCE_DIR}/src/base_shader.cpp
)

add_library(bsp STATIC
	${BSP_PUBLIC_HEADERS}
	${BSP_PRIVATE_HEADERS}
	${BSP_PRIVATE_SOURCES}
)

target_include_directories(bsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(bsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(bsp PUBLIC
	GLM_FORCE_SILENT_WARNINGS
)

target_link_libraries(bsp
	appfw
	glm::glm
)

source_group("Public Headers" FILES ${RNDR_PUBLIC_HEADERS})
source_group("Private Headers" FILES ${RNDR_PRIVATE_HEADERS})
source_group("Private Sources" FILES ${RNDR_PRIVATE_SOURCES})
