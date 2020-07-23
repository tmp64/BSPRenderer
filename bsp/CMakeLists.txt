set(BSP_PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/bsp_types.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/level.h

)

set(BSP_PRIVATE_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/src/this_module_info.h
)

set(BSP_PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/level.cpp
)

add_library(bsp STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
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

source_group("Public Headers" FILES ${BSP_PUBLIC_HEADERS})
source_group("Private Headers" FILES ${BSP_PRIVATE_HEADERS})
source_group("Private Sources" FILES ${BSP_PRIVATE_SOURCES})
