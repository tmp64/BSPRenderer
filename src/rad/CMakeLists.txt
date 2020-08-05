add_executable(rad
	CMakeLists.txt
	src/bsp_tree.cpp
	src/bsp_tree.h
	src/main.cpp
	src/main.h
	src/patches.cpp
	src/patches.h
	src/this_module_info.h
	src/utils.cpp
	src/utils.h
)

target_include_directories(rad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(rad
	appfw
	bsp
)
