add_executable(bspviewer
	CMakeLists.txt
	src/app.cpp
	src/app.h
	src/bsp_tree.cpp
	src/bsp_tree.h
	src/demo.cpp
	src/demo.h
	src/dev_console_dialog.cpp
	src/dev_console_dialog.h
	src/input_system.cpp
	src/input_system.h
	src/main.cpp
	src/this_module_info.h
)

target_include_directories(bspviewer PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bspviewer
	appfw
	glad
	Freetype::Freetype
	imgui
	renderer
	SDL2::SDL2
	SDL2::SDL2main
)
