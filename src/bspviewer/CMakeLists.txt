add_executable(bspviewer
	CMakeLists.txt
	src/base_entity.cpp
	src/base_entity.h
	src/brush_model.cpp
	src/brush_model.h
	src/bsp_tree.cpp
	src/bsp_tree.h
	src/bspviewer.cpp
	src/bspviewer.h
	src/demo.cpp
	src/demo.h
	src/renderer.cpp
	src/renderer.h
	src/this_module_info.h
	src/world_state.cpp
	src/world_state.h
)

target_include_directories(bspviewer PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bspviewer
	gui_app
)
