set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt

	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/app_base.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/app_component.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/app_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/components.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/lightmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/texture_block.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/app_base/yaml.h

	${CMAKE_CURRENT_SOURCE_DIR}/src/app_base.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app_component.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app_config.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/components.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/texture_block.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/yaml.cpp
)

add_library(app_base STATIC
	${SOURCE_FILES}
)

appfw_module(app_base)
target_include_directories(app_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(app_base PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(app_base
	appfw
	glm::glm
	taskflow
	yaml-cpp
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(app_base PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
	<taskflow/taskflow.hpp>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
