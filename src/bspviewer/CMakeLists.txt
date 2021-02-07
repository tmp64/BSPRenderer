add_executable(bspviewer
	CMakeLists.txt
	src/bsp_tree.cpp
	src/bsp_tree.h
	src/bspviewer.cpp
	src/bspviewer.h
	src/demo.cpp
	src/demo.h
	src/this_module_info.h
)

target_include_directories(bspviewer PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bspviewer
	gui_app
)
