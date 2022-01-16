set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_impl_opengl3.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_impl_sdl.h
	
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_opengl3.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_sdl.cpp
)

add_library(imgui_impl STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${SOURCE_FILES}
)

appfw_module(imgui_impl)
target_include_directories(imgui_impl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(imgui_impl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(imgui_impl PRIVATE
	IMGUI_IMPL_OPENGL_LOADER_CUSTOM
)

target_link_libraries(imgui_impl
	material_system
	imgui
	SDL2::SDL2
)

target_precompile_headers(imgui_impl PRIVATE
    ${PCH_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
