set(SOURCE_FILES
	CMakeLists.txt
	src/assets/asset.cpp
	src/assets/asset.h
	src/assets/asset_future.h
	src/assets/asset_manager.cpp
	src/assets/asset_manager.h
	src/assets/level_asset.cpp
	src/assets/level_asset.h
	src/assets/wad_asset.cpp
	src/assets/wad_asset.h
	src/assets/wad_material_asset.cpp
	src/assets/wad_material_asset.h

	src/base_entity.cpp
	src/base_entity.h
	src/brush_model.cpp
	src/brush_model.h
	src/bspviewer.cpp
	src/bspviewer.h
	src/renderer.cpp
	src/renderer.h
	src/world_state.cpp
	src/world_state.h
)

add_executable(bspviewer
	${SOURCE_FILES}
	../gui_app_base/src/gui_app.manifest
)

appfw_module(bspviewer)

target_include_directories(bspviewer PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bspviewer
	gui_app_base
	app_gui_launcher
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(bspviewer PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
