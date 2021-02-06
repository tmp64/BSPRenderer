project(SDL_net)

add_library(sdl_net STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${CMAKE_CURRENT_SOURCE_DIR}/src/SDL_net.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/SDLnet.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/SDLnetselect.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/SDLnetsys.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/SDLnetTCP.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/SDLnetUDP.c
)

#target_compile_definitions(sdl_net PUBLIC WITHOUT_SDL)
target_include_directories(sdl_net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(PLAT_LIBS)

if(PLATFORM_WINDOWS)
	set(PLAT_LIBS
		${PLAT_LIBS}
		Ws2_32.lib
		Iphlpapi.lib
	)
endif()

target_link_libraries(sdl_net ${PLAT_LIBS} SDL2::SDL2)
