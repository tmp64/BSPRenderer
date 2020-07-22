set(BSP_PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/base_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/bsp_types.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/edges_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/entity_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/faces_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/leaves_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/level.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/marksurfaces_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/nodes_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/planes_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/surfedges_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/texinfo_lump.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/bsp/vertices_lump.h

)

set(BSP_PRIVATE_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/src/this_module_info.h
)

set(BSP_PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	
	${CMAKE_CURRENT_SOURCE_DIR}/src/base_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/edges_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/entity_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/faces_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/leaves_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/level.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/marksurfaces_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/nodes_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/planes_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/surfedges_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/texinfo_lump.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vertices_lump.cpp
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

source_group("Public Headers" FILES ${BSP_PUBLIC_HEADERS})
source_group("Private Headers" FILES ${BSP_PRIVATE_HEADERS})
source_group("Private Sources" FILES ${BSP_PRIVATE_SOURCES})
