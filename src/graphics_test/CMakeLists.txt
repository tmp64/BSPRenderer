set(SOURCE_FILES
	CMakeLists.txt
	src/graphics_test_app.cpp
	src/graphics_test_app.h
)

add_executable(graphics_test
	${SOURCE_FILES}
	../gui_app_base/src/gui_app.manifest
)

appfw_module(graphics_test)

target_include_directories(graphics_test PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(graphics_test
	gui_app_base
	app_gui_launcher
	graphics
)

target_precompile_headers(graphics_test PRIVATE
    ${PCH_GUI_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
