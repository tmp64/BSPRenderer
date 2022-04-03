set(SOURCE_FILES
	CMakeLists.txt
	src/dialog_base.cpp
	src/dialog_base.h
	src/file_browser.cpp
	src/file_browser.h
	src/hlviewer.cpp
	src/hlviewer.h
	src/map_viewer.cpp
	src/map_viewer.h
)

add_executable(hlviewer
	${SOURCE_FILES}
	../gui_app_base/src/gui_app.manifest
)

appfw_module(hlviewer)

target_include_directories(hlviewer PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(hlviewer
	gui_app_base
	app_gui_launcher
	hlviewer_shared
	renderer
)

target_precompile_headers(hlviewer PRIVATE
    ${PCH_GUI_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
