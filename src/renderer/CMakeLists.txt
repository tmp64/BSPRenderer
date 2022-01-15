set(RNDR_PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/client_entity.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/const.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/model.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/renderer_engine_interface.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/scene_renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/scene_shaders.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/surface_renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/utils.h
)

set(RNDR_PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/scene_renderer.cpp
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
	RENDERER_SUPPORT_TINTING
)

target_link_libraries(renderer
	appfw
	bsp
	gui_app_base
	imgui
	stb
	material_system
)

target_precompile_headers(renderer PRIVATE
    ${PCH_GUI_APP_BASE_HEADERS}
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
