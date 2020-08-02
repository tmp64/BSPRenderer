add_executable(playground
	../../CMakeLists.txt
	CMakeLists.txt
	src/main.cpp
	src/this_module_info.h
)

target_include_directories(playground PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(playground
	appfw
	bsp
	renderer
)
