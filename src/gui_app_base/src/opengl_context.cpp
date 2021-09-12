#include "SDL.h"
#include <gui_app_base/gui_app_base.h>
#include <gui_app_base/opengl_context.h>

static ConVar<bool> gl_break_on_error("gl_break_on_error", true,
                                      "Break in debugger on OpenGL error");

#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 2
#endif

void OpenGLContext::setupWindowAttributes() {
    if (SDL_GL_LoadLibrary(nullptr)) {
        app_fatalError("Failed to load OpenGL library: {}", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int contextFlags = 0;

    bool bNoDebug = getCommandLine().isFlagSet("--gl-no-debug");
    bool bDebug = appfw::isDebugBuild() || getCommandLine().isFlagSet("--gl-debug");

    if (bDebug && !bNoDebug) {
        contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);
}

OpenGLContext::OpenGLContext() {
    SDL_Window *pWindow = MainWindowComponent::get().getWindow();

    // Create OpenGL context
    m_GLContext = SDL_GL_CreateContext(pWindow);

    if (!m_GLContext) {
        app_fatalError("Failed to create OpenGL context: ", SDL_GetError());
    }

    // Load OpenGL functions
    if (!gladLoadGLLoader([](const char *name) { return SDL_GL_GetProcAddress(name); })) {
        app_fatalError("Failed load OpenGL functions.");
    }

    if (!GLAD_GL_VERSION_3_3) {
        app_fatalError("At least OpenGL 3.3 is required. Available {}.{}", GLVersion.major,
                       GLVersion.minor);
    }

    glad_set_post_callback(gladPostCallback);

    printi("OpenGL: Version {}.{} loaded", GLVersion.major, GLVersion.minor);
    printi("GL_VENDOR: {}", glGetString(GL_VENDOR));
    printi("GL_RENDERER: {}", glGetString(GL_RENDERER));
    printi("GL_VERSION: {}", glGetString(GL_VERSION));

    int contextFlags = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
    bool bDebugRequested = contextFlags & SDL_GL_CONTEXT_DEBUG_FLAG;

    int glProfileMask = 0;
    int glContextFlags = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &glProfileMask);
    glGetIntegerv(GL_CONTEXT_FLAGS, &glContextFlags);

    if (glProfileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {
        printw("OpenGL: Requested core profile, got compatibility instead");
    }

    if (glContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        if (GLAD_GL_ARB_debug_output) {
            printi("OpenGL: Using debug context");
        } else {
            printw("OpenGL: Got debug context but ARB_debug_output is not supported");
        }
    } else if (bDebugRequested) {
        printw("OpenGL: Debug context requested but not available");
    }

    if (GLAD_GL_ARB_debug_output) {
        glDebugMessageCallbackARB(&debugMsgCallback, nullptr);
    }
}

OpenGLContext::~OpenGLContext() {
    SDL_GL_DeleteContext(m_GLContext);
    m_GLContext = nullptr;
}

void OpenGLContext::gladPostCallback(const char *name, void *, int, ...) {
    GLenum errorCode = glad_glGetError();

    if (errorCode != GL_NO_ERROR) {
        printe("OpenGL Error: {} (0x{:X}) in {}", getGlErrorString(errorCode), (int)errorCode,
               name);
        if (gl_break_on_error.getValue()) {
            AFW_DEBUG_BREAK();
        }
    }
}

const char *OpenGLContext::getGlErrorString(GLenum errorCode) {
    switch (errorCode) {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "< unknown >";
    }
}

void OpenGLContext::debugMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                     GLsizei length, const GLchar *message, const void *) {
    std::string_view sourceStr;
    std::string_view typeStr;
    std::string_view severityStr;
    ConMsgType conMsgType = ConMsgType::Warn;

    //----------------------------------------
    switch (source) {
    case GL_DEBUG_SOURCE_API_ARB: {
        sourceStr = "GL API";
        break;
    }
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: {
        sourceStr = "Window System";
        break;
    }
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: {
        sourceStr = "Shader Compiler";
        break;
    }
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: {
        sourceStr = "Third-Party";
        break;
    }
    case GL_DEBUG_SOURCE_APPLICATION_ARB: {
        sourceStr = "Application";
        break;
    }
    case GL_DEBUG_SOURCE_OTHER_ARB: {
        sourceStr = "Other";
        break;
    }
    default: {
        sourceStr = "Unknown";
        break;
    }
    }

    //----------------------------------------
    switch (type) {
    case GL_DEBUG_TYPE_ERROR_ARB: {
        typeStr = "Error";
        break;
    }
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: {
        typeStr = "Deprecated Behavior";
        break;
    }
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: {
        typeStr = "Undefined Behavior";
        break;
    }
    case GL_DEBUG_TYPE_PORTABILITY_ARB: {
        typeStr = "Portability";
        break;
    }
    case GL_DEBUG_TYPE_PERFORMANCE_ARB: {
        typeStr = "Performance";
        break;
    }
    case GL_DEBUG_TYPE_OTHER_ARB: {
        typeStr = "Other";
        break;
    }
    default: {
        typeStr = "Unknown";
        break;
    }
    }

    //----------------------------------------
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH_ARB: {
        severityStr = "High";
        conMsgType = ConMsgType::Error;
        break;
    }
    case GL_DEBUG_SEVERITY_MEDIUM_ARB: {
        severityStr = "Medium";
        break;
    }
    case GL_DEBUG_SEVERITY_LOW_ARB: {
        severityStr = "Low";
        break;
    }
    default: {
        severityStr = "Unknown";
        break;
    }
    }

    //----------------------------------------
    printtype(conMsgType, "OpenGL: {}: {}", id, std::string_view(message, length));
    printtype(conMsgType, "OpenGL: Source: {}", sourceStr);
    printtype(conMsgType, "OpenGL: Type: {}", typeStr);
    printtype(conMsgType, "OpenGL: Severity: {}", severityStr);
}
