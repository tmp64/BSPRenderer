set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_system/material.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_system/material_system.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_system/shader.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_system/shader_definitions.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_system/shader_instance.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_system/shader_types.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/material_system/shader_uniform.h
	
	${CMAKE_CURRENT_SOURCE_DIR}/src/material.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/material_system.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/shader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/shader_definitions.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/shader_instance.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/shader_uniform.cpp
)

add_library(material_system STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${SOURCE_FILES}
)

appfw_module(material_system)
target_include_directories(material_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(material_system PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(material_system
	graphics
	stb
	imgui
)

target_precompile_headers(material_system PRIVATE
    ${PCH_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
