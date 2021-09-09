add_library(app_gui_launcher STATIC
	CMakeLists.txt
	src/main.cpp
)

appfw_module(app_gui_launcher)
target_include_directories(app_gui_launcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(app_gui_launcher
	app_base
	SDL2::SDL2
	SDL2::SDL2main
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(app_gui_launcher PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
)
