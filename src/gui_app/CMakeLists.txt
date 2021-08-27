set(PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app/base_view.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app/base_view_2d.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app/dev_console_dialog.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app/dev_console_overlay.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app/gui_app_base.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app/imgui_controls.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app/input_system.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app/profiler_dialog.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app/utils.h
)

set(PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/base_view.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/base_view_2d.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/dev_console_dialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/dev_console_overlay.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui_app_base.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/input_system.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/profiler_dialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp
)

add_library(gui_app STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${PUBLIC_HEADERS}
	${PRIVATE_SOURCES}
)

appfw_module(gui_app)
target_include_directories(gui_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(gui_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(gui_app
	appfw
	app_base
	glad
	glm::glm
	imgui
	renderer
	SDL2::SDL2
	SDL2::SDL2main
	nlohmann_json
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(gui_app PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
)

source_group("Public Headers" FILES ${PUBLIC_HEADERS})
source_group("Private Headers" FILES ${PRIVATE_HEADERS})
source_group("Private Sources" FILES ${PRIVATE_SOURCES})
