add_executable(rad
	CMakeLists.txt
	src/main.cpp
	src/main.h
	src/patch_list.cpp
	src/patch_list.h
	src/patches.cpp
	src/patches.h
	src/plat_win.cpp
	src/plat.h
	src/this_module_info.h
	src/utils.cpp
	src/utils.h
	src/vis.cpp
	src/vis.h
)

target_include_directories(rad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(rad
	appfw
	bsp
)
