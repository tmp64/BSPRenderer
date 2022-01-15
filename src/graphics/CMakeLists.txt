set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/framebuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/gpu_buffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/graphics_stack.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/raii.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/render_buffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/render_target.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/shader_program.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/shader_stage.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/texture.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/texture_cube.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/texture2d.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/graphics/vram_tracker.h
	
	${CMAKE_CURRENT_SOURCE_DIR}/src/framebuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gpu_buffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics_stack.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/render_buffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/shader_program.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/shader_stage.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/texture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/texture_cube.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/texture2d.cpp
)

add_library(graphics STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${SOURCE_FILES}
)

appfw_module(graphics)
target_include_directories(graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(graphics PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(graphics PUBLIC
	GLM_FORCE_SILENT_WARNINGS
)

target_link_libraries(graphics
	app_base
	glad
)

target_precompile_headers(graphics PRIVATE
    ${PCH_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
