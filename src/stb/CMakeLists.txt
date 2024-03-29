set(PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/stb_image.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/stb_image_write.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/stb_include.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/stb_rect_pack.h
)

set(PRIVATE_HEADERS
)

set(PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image_write.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/stb_include.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/stb_rect_pack.cpp
)

add_library(stb STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${PUBLIC_HEADERS}
	${PRIVATE_HEADERS}
	${PRIVATE_SOURCES}
)

appfw_module(stb)
target_include_directories(stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(stb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(stb
	appfw
)

source_group("Public Headers" FILES ${PUBLIC_HEADERS})
source_group("Private Headers" FILES ${PRIVATE_HEADERS})
source_group("Private Sources" FILES ${PRIVATE_SOURCES})
