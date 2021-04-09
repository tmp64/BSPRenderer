set(PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/app_config.h
)

set(PRIVATE_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/src/this_module_info.h
)

set(PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/app_config.cpp
)

add_library(app_base STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${PUBLIC_HEADERS}
	${PRIVATE_HEADERS}
	${PRIVATE_SOURCES}
)

target_include_directories(app_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(app_base PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(app_base
	appfw
	nlohmann_json
)

source_group("Public Headers" FILES ${PUBLIC_HEADERS})
source_group("Private Headers" FILES ${PRIVATE_HEADERS})
source_group("Private Sources" FILES ${PRIVATE_SOURCES})