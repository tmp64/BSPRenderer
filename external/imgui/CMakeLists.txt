project(imgui)

add_library(imgui STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/imconfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_demo.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_draw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_internal.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_widgets.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imstb_rectpack.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/imstb_textedit.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/imstb_truetype.h
	
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_opengl3.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_opengl3.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_sdl.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_sdl.h
)

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(imgui PUBLIC
	IMGUI_IMPL_OPENGL_LOADER_GLAD
)

target_link_libraries(imgui
	glad
	SDL2::SDL2
)
