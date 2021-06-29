add_executable(playground
	../../CMakeLists.txt
	CMakeLists.txt
	src/bmplib.hpp
	src/main.cpp
	src/this_module_info.h
)

appfw_module(playground)
target_include_directories(playground PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(playground
	appfw
	renderer
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(playground PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
)
