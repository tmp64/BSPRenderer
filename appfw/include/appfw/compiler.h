#ifndef APPFW_COMPILER_H
#define APPFW_COMPILER_H
#include <appfw/platform.h>

#ifdef DOXYGEN

/**
 * Hit a breakpoint at the point of this macro.
 */
#define AFW_DEBUG_BREAK /* implementation defined */

/**
 * Force the compiler to inline the function.
 * Does not always work (depends on compiler).
 */
#define AFW_FORCE_INLINE /* implementation defined */

#elif COMPILER_MSVC

#include <intrin.h>

#define AFW_DEBUG_BREAK() __debugbreak()
#define AFW_FORCE_INLINE __forceinline

#elif COMPILER_GNU

#endif

#endif
