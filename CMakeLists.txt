cmake_minimum_required(VERSION 3.16.0)

project(BSPRenderer)

include(FetchContent)
include(external/appfw/cmake/platform_info.cmake)

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
message(STATUS ">>>>>> Dependencies begin")
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)

FetchContent_Declare(
	yaml_cpp_fetch
	GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
	GIT_TAG "yaml-cpp-0.7.0"
	GIT_SHALLOW
	#CMAKE_ARGS "-DYAML_CPP_BUILD_CONTRIB=OFF -DYAML_CPP_BUILD_TOOLS=OFF -DYAML_BUILD_SHARED_LIBS=OFF -DYAML_CPP_BUILD_TESTS=OFF"
)

set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml_cpp_fetch)

add_subdirectory(external/appfw)
add_subdirectory(external/glad_debug)
add_subdirectory(external/imgui)
add_subdirectory(external/nlohmann_json)
add_subdirectory(external/taskflow)
message(STATUS ">>>>>> Dependencies end")

# Libraries
add_subdirectory(src/app_base)
add_subdirectory(src/gui_app)
add_subdirectory(src/bsp)
add_subdirectory(src/rad)
add_subdirectory(src/renderer)

# Executables
add_subdirectory(src/bspviewer)
add_subdirectory(src/playground)
add_subdirectory(src/rad_launcher)
