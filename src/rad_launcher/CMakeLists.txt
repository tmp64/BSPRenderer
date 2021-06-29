add_executable(rad_launcher
	CMakeLists.txt
	src/main.cpp
	src/this_module_info.h
)

appfw_module(rad_launcher)
target_include_directories(rad_launcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(rad_launcher
	rad
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(rad_launcher PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
)
