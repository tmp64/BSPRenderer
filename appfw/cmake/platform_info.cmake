# This file sets following variables and macros to TRUE when building for...
#
#	PLATFORM_WINDOWS			Windows
#	PLATFORM_UNIX				any Unix-like OS
#		PLATFORM_LINUX				any OS with Linux kernel
#			PLATFORM_ANDROID		Android specifically
#		PLATFORM_MACOS				macOS
#
#
#
# This file sets following variables and macros to TRUE when building with...
#
#	COMPILER_MSVC				MS Visual Studio
#	COMPILER_GNU				any GCC-compatible compiler
#		COMPILER_GCC			GCC
#		COMPILER_CLANG			LLVM/Clang
#
# Macros are always defined but their value can be 0 or 1.
# They are added to ${PLATFORM_DEFINES}

if( APPFW_PLATFORMINFO_INCLUDED )
	return()
endif()
set( APPFW_PLATFORMINFO_INCLUDED true )

set(PLATFORM_WINDOWS FALSE)
set(PLATFORM_UNIX FALSE)
set(PLATFORM_LINUX FALSE)
set(PLATFORM_ANDROID FALSE)
set(PLATFORM_MACOS FALSE)

set(COMPILER_MSVC FALSE)
set(COMPILER_GNU FALSE)
set(COMPILER_GCC FALSE)
set(COMPILER_CLANG FALSE)

set(PLATFORM_DEFINES)

#-----------------------------------------------------------------------
# Platform identification
#-----------------------------------------------------------------------
if(WIN32)

	# Windows
	set(PLATFORM_WINDOWS TRUE)
	
elseif(UNIX)
	set(PLATFORM_UNIX TRUE)
	
	if(APPLE)
		# macOS
		set(PLATFORM_MACOS TRUE)
	else()
		# Linux kernel (most likely, could be BSD)
		# TODO: Add BSD & others support
		set(PLATFORM_LINUX TRUE)
		
		if(ANDROID)
			set(PLATFORM_ANDROID TRUE)
		endif()
	endif()
	
else()
	message(FATAL_ERROR "Platform identification failed. Please add your platform to cmake/PlatformInfo.cmake")
endif()

#-----------------------------------------------------------------------
# Compiler identification
#-----------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
	
	# GNU
	set(COMPILER_GNU TRUE)
	
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		# GCC
		set(COMPILER_GCC TRUE)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		# Clang
		set(COMPILER_CLANG TRUE)
	else()
		message(FATAL_ERROR "This isn't supposed to happen.")
	endif()
	
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# MSVC
	set(COMPILER_MSVC TRUE)
else()
	message(WARNING "Unknown compiler deteceted: ${CMAKE_CXX_COMPILER_ID}. Defaulting to GNU-compatible.")
	set(COMPILER_GNU TRUE)
endif()

#-----------------------------------------------------------------------
# Preprocessor definitions
#-----------------------------------------------------------------------
macro(platform_add_definition var)
	if(${var})
		set(PLATFORM_DEFINES ${PLATFORM_DEFINES} ${var}=1)
	else()
		set(PLATFORM_DEFINES ${PLATFORM_DEFINES} ${var}=0)
	endif()
endmacro()

platform_add_definition(PLATFORM_WINDOWS)
platform_add_definition(PLATFORM_UNIX)
platform_add_definition(PLATFORM_LINUX)
platform_add_definition(PLATFORM_ANDROID)
platform_add_definition(PLATFORM_MACOS)
platform_add_definition(COMPILER_MSVC)
platform_add_definition(COMPILER_GNU)
platform_add_definition(COMPILER_GCC)
platform_add_definition(COMPILER_CLANG)

if(COMPILER_MSVC)
	# Silent stupid warnings that say to use non-standart stuff
	set(PLATFORM_DEFINES ${PLATFORM_DEFINES} _CRT_SECURE_NO_WARNINGS)
endif()

