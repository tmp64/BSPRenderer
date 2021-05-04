cmake_minimum_required(VERSION 3.16.0)

project(BSPRenderer)

include(src/appfw/cmake/platform_info.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(COMPILER_MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /W4")

	# Disable RTC in debug since it interferes with texture block optimization
	string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

# External libraries
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
add_subdirectory(external/imgui)
add_subdirectory(external/nlohmann_json)
add_subdirectory(external/sdl_net)

# Libraries
add_subdirectory(src/appfw)
add_subdirectory(src/app_base)
add_subdirectory(src/gui_app)
add_subdirectory(src/bsp)
add_subdirectory(src/rad)
add_subdirectory(src/renderer)

# Executables
add_subdirectory(src/bspviewer)
add_subdirectory(src/playground)
add_subdirectory(src/rad_launcher)
