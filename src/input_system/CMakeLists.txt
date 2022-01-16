set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/input/input_system.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/input/key_bind.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/input/key_codes.h
	
	${CMAKE_CURRENT_SOURCE_DIR}/src/input_system.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/key_bind.cpp
)

add_library(input_system STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${SOURCE_FILES}
)

appfw_module(input_system)
target_include_directories(input_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(input_system PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(input_system
	app_base
	imgui
	imgui_impl
	SDL2::SDL2
)

target_precompile_headers(input_system PRIVATE
    ${PCH_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
