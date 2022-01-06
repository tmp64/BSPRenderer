set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app_base/base_view.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app_base/base_view_2d.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app_base/config_component.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app_base/config_item.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app_base/gui_app_base.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app_base/gui_components.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app_base/imgui_controls.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gui_app_base/opengl_context.h

	${CMAKE_CURRENT_SOURCE_DIR}/src/base_view.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/base_view_2d.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/config_component.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/config_item.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/dev_console_dialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/dev_console_dialog.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui_app_base.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui_components.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/opengl_context.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/profiler_dialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/profiler_dialog.h
)

add_library(gui_app_base STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${SOURCE_FILES}
)

appfw_module(gui_app_base)
target_include_directories(gui_app_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(gui_app_base PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(gui_app_base
	appfw
	app_base
	glad
	glm::glm
	imgui
	input_system
	renderer
	SDL2::SDL2
	SDL2::SDL2main
)

target_precompile_headers(gui_app_base PRIVATE
    ${PCH_GUI_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
