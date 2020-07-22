project(appfw)

include(cmake/platform_info.cmake)

set(APPFW_PUBLIC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/console/con_item.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/console/console_system.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/console/std_console.h

	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/log/base_logger.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/log/console_logger.h

	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/color.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/command_buffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/compiler.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/dbg.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/init.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/module_info.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/platform.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/services.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/timer.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/appfw/utils.h
)

set(APPFW_PRIVATE_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/src/this_module_info.h
)

set(APPFW_PRIVATE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt

	${CMAKE_CURRENT_SOURCE_DIR}/src/console/con_item.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/console/console_system.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/console/std_console.cpp
	
	${CMAKE_CURRENT_SOURCE_DIR}/src/log/base_logger.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/log/console_logger.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/src/command_buffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/dbg.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/init.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp
)

add_library(appfw STATIC
	${APPFW_PUBLIC_HEADERS}
	${APPFW_PRIVATE_HEADERS}
	${APPFW_PRIVATE_SOURCES}
)

target_include_directories(appfw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(appfw PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(appfw PUBLIC ${PLATFORM_DEFINES})

target_link_libraries(appfw fmt)

source_group("Public Headers" FILES ${APPFW_PUBLIC_HEADERS})
source_group("Private Headers" FILES ${APPFW_PRIVATE_HEADERS})
source_group("Private Sources" FILES ${APPFW_PRIVATE_SOURCES})

if(BUILD_TESTING)
	set(APPFW_TEST_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp
	)
	
	add_executable(appfw_test_exec ${APPFW_TEST_SOURCES})
	target_link_libraries(appfw_test_exec appfw)
	add_test(appfw_test appfw_test_exec)
endif()