set(PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/app_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/yaml.h
)

set(PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/app_config.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/yaml.cpp
)

add_library(app_base STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${PUBLIC_HEADERS}
	${PRIVATE_SOURCES}
)

appfw_module(app_base)
target_include_directories(app_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(app_base PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(app_base
	appfw
	glm::glm
	nlohmann_json
	yaml-cpp
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(app_base PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
)

source_group("Public Headers" FILES ${PUBLIC_HEADERS})
source_group("Private Headers" FILES ${PRIVATE_HEADERS})
source_group("Private Sources" FILES ${PRIVATE_SOURCES})
