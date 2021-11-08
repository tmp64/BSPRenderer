set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_props/base_material_props.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_props/material_prop_loader.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_props/material_props.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_props/sound_materials.h
	
	${CMAKE_CURRENT_SOURCE_DIR}/src/base_material_props.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/material_prop_loader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/material_props.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/sound_materials.cpp
)

add_library(material_props STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${SOURCE_FILES}
)

appfw_module(material_props)
target_include_directories(material_props PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(material_props PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(material_props
	app_base
	glm::glm
)

appfw_get_std_pch(PCH_STD_HEADERS)
appfw_get_pch(PCH_APPFW_HEADERS)
target_precompile_headers(material_props PRIVATE
    ${PCH_STD_HEADERS}
    ${PCH_APPFW_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
