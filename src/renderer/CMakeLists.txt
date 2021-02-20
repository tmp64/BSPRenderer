set(RNDR_PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/base_shader.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/font_renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/frame_console.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/material_manager.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/raii.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/scene_renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/shader_manager.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/surface_renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/utils.h
)

set(RNDR_PRIVATE_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/src/this_module_info.h
)

set(RNDR_PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/base_shader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/font_renderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/frame_console.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/material_manager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/scene_renderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/shader_manager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/surface_renderer.cpp
)

add_library(renderer STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${RNDR_PUBLIC_HEADERS}
	${RNDR_PRIVATE_HEADERS}
	${RNDR_PRIVATE_SOURCES}
)

target_include_directories(renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(renderer PUBLIC
	GLM_FORCE_SILENT_WARNINGS
)

target_link_libraries(renderer
	appfw
	bsp
	glad
	glm::glm
	imgui
	Freetype::Freetype
)

source_group("Public Headers" FILES ${RNDR_PUBLIC_HEADERS})
source_group("Private Headers" FILES ${RNDR_PRIVATE_HEADERS})
source_group("Private Sources" FILES ${RNDR_PRIVATE_SOURCES})
