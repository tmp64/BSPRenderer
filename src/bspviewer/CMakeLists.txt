set(SOURCE_FILES
	CMakeLists.txt

	src/entity_editor/entity_edit_mode.cpp
	src/entity_editor/entity_edit_mode.h
	src/entity_editor/entity_select_tool.cpp
	src/entity_editor/entity_select_tool.h

	src/surface_editor/files.h
	src/surface_editor/surface_edit_mode.cpp
	src/surface_editor/surface_edit_mode.h
	src/surface_editor/surface_select_tool.cpp
	src/surface_editor/surface_select_tool.h
	
	src/bspviewer.cpp
	src/bspviewer.h
	src/editor_mode.cpp
	src/editor_mode.h
	src/editor_tool.cpp
	src/editor_tool.h
	src/main_view.cpp
	src/main_view.h
	src/world_state.cpp
	src/world_state.h
)

add_executable(bspviewer
	${SOURCE_FILES}
	../gui_app_base/src/gui_app.manifest
)

appfw_module(bspviewer)

target_include_directories(bspviewer PRIVATE
	src
)

target_link_libraries(bspviewer
	gui_app_base
	app_gui_launcher
	material_props
	hlviewer_shared
)

target_precompile_headers(bspviewer PRIVATE
    ${PCH_GUI_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
