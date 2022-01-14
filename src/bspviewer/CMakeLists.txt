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
	
	src/surface_editor/files.h
	src/surface_editor/surface_edit_mode.cpp
	src/surface_editor/surface_edit_mode.h
	src/surface_editor/surface_select_tool.cpp
	src/surface_editor/surface_select_tool.h

	src/base_entity.cpp
	src/base_entity.h
	src/brush_model.cpp
	src/brush_model.h
	src/bspviewer.cpp
	src/bspviewer.h
	src/editor_mode.cpp
	src/editor_mode.h
	src/editor_tool.cpp
	src/editor_tool.h
	src/level_loader.cpp
	src/level_loader.h
	src/main_view_renderer.cpp
	src/main_view_renderer.h
	src/vis.cpp
	src/vis.h
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
	material_props
	renderer
)

target_precompile_headers(bspviewer PRIVATE
    ${PCH_GUI_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
