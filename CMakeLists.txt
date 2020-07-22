cmake_minimum_required(VERSION 3.16.0)

project(hl_advgl)

include(appfw/cmake/platform_info.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(COMPILER_MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /EHsc /W4")
endif()

find_package(SDL2 REQUIRED)
find_package(Freetype REQUIRED)
find_package(glm REQUIRED)

if(NOT FREETYPE_FOUND)
	message(FATAL_ERROR "Freetype not found")
endif()

message(STATUS ">>> FMT library begin")
add_subdirectory(external/fmt)
message(STATUS ">>> FMT library end")
add_subdirectory(external/glad_debug)

add_subdirectory(appfw)
add_subdirectory(bsp)
add_subdirectory(renderer)

add_subdirectory(bspviewer)
add_subdirectory(playground)
