project(glad)

set(GLAD_PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/KHR/khrplatform.h
)

set(GLAD_PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
)

add_library(glad STATIC
	${GLAD_PUBLIC_HEADERS}
	${GLAD_PRIVATE_SOURCES}
)

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(glad PUBLIC GLAD_OPENGL_ES IMGUI_IMPL_OPENGL_ES2)
