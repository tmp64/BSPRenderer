#ifndef APPFW_PLATFORM_H
#define APPFW_PLATFORM_H

namespace appfw {

namespace platform {

//----------------------------------------------------------------

#if defined(_DEBUG) || defined(DEBUG)
#define AFW_DEBUG_BUILD 1
#else
#define AFW_DEBUG_BUILD 0
#endif

/**
 * Returns true if this is a debug build.
 */
constexpr bool isDebugBuild() { return AFW_DEBUG_BUILD; }

//----------------------------------------------------------------

/**
 * Returns true when building for Windows.
 */
constexpr bool isWindows() { return PLATFORM_WINDOWS; }

/**
 * Returns true when building for a UNIX-like OS.
 */
constexpr bool isUnix() { return PLATFORM_UNIX; }

/**
 * Returns true when building for Linux.
 */
constexpr bool isLinux() { return PLATFORM_LINUX; }

/**
 * Returns true when building for Android.
 */
constexpr bool isAndroid() { return PLATFORM_ANDROID; }

/**
 * Returns true when building for macOS.
 */
constexpr bool isMacOS() { return PLATFORM_MACOS; }

// Check that OS is correct
// Make sure to update `getOSName` as well.
static_assert(isWindows() || isUnix() || isMacOS(), "Unknown target OS");

//----------------------------------------------------------------

/**
 * Returns true when compiling with Visual C++ compiler.
 */
constexpr bool isMSVC() { return COMPILER_MSVC; }

/**
 * Returns true when compiling with GCC-compatible compiler.
 */
constexpr bool isGNU() { return COMPILER_GNU; }

/**
 * Returns true when compiling with GCC.
 */
constexpr bool isGCC() { return COMPILER_GCC; }

/**
 * Returns true when compiling with Clang.
 */
constexpr bool isClang() { return COMPILER_CLANG; }

// Check that compiler is correct
// Make sure to update `getCompilerName` as well.
static_assert(isMSVC() || isGNU(), "Unknown compiler");

//----------------------------------------------------------------

/**
 * Returns the name of the OS app is built for.
 */
constexpr const char *getOSName() {
    if (isWindows()) {
        return "Windows";
    }

    if (isLinux()) {
        if (isAndroid()) {
            return "Android";
        }

        return "Linux";
    }

    if (isMacOS()) {
        return "macOS";
    }

    return "Unknown OS";
}

/**
 * Returns the name of the compiler app is built with.
 */
constexpr const char *getCompilerName() {
    if (isMSVC()) {
        return "Visual C++";
    }

    if (isGNU()) {
        if (isGCC()) {
            return "GCC";
        }

        if (isClang()) {
            return "Clang";
        }

        return "GNU-compatible";
    }

    return "Unknown compiler";
}

/**
 * Returns date and time when this translation unit was built.
 * Format: Month DD YYYY HH:MM:SS
 * e.g. Jul 19 2020 19:41:12
 */
constexpr const char *getBuildDate() { return __DATE__ " " __TIME__; }

} // namespace platform

} // namespace appfw

#endif
