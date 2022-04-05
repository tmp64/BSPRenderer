set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/client_entity.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/const.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/envmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/model.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/renderer_engine_interface.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/scene_renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/scene_shaders.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/sprite_model.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/utils.h

	${CMAKE_CURRENT_SOURCE_DIR}/src/brush_renderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/brush_renderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/bsp_lightmap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/bsp_lightmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/custom_lightmap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/custom_lightmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/envmap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/fake_lightmap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/fake_lightmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/lightmap_iface.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/scene_renderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/world_renderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/world_renderer.h
)

add_library(renderer STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${SOURCE_FILES}
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

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
