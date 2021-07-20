set(RNDR_PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/base_shader.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/client_entity.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/const.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/material_manager.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/model.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/raii.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/scene_renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/shader_manager.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/stb_image.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/surface_renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/utils.h
)

set(RNDR_PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/base_shader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/material_manager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/scene_renderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/shader_manager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/surface_renderer.cpp
)

add_library(renderer STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${RNDR_PUBLIC_HEADERS}
	${RNDR_PRIVATE_SOURCES}
)

appfw_module(renderer)
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
	gui_app
	imgui
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(renderer PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
)

# TextureBlock is extremely slow without optiomizations
if (COMPILER_MSVC)
	set_source_files_properties( 
		${CMAKE_CURRENT_SOURCE_DIR}/src/texture_block.cpp
		DIRECTORY ./
		PROPERTIES
		COMPILE_FLAGS "/O2 /Ob2"
		SKIP_PRECOMPILE_HEADERS TRUE
	)
elseif (COMPILER_GNU)
	set_source_files_properties( 
		${CMAKE_CURRENT_SOURCE_DIR}/src/texture_block.cpp
		DIRECTORY ./
		PROPERTIES
		COMPILE_FLAGS "-O2"
		SKIP_PRECOMPILE_HEADERS TRUE
	)
endif()

source_group("Public Headers" FILES ${RNDR_PUBLIC_HEADERS})
source_group("Private Headers" FILES ${RNDR_PRIVATE_HEADERS})
source_group("Private Sources" FILES ${RNDR_PRIVATE_SOURCES})
