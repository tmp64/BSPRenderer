add_executable(rad_launcher
	CMakeLists.txt
	src/main.cpp
	src/this_module_info.h
)

target_include_directories(rad_launcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(rad_launcher
	rad
)
