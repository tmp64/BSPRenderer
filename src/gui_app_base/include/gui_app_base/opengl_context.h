#ifndef GUI_APP_BASE_OPENGL_CONTEXT_H
#define GUI_APP_BASE_OPENGL_CONTEXT_H
#include <glad/glad.h>
#include <app_base/app_component.h>

class OpenGLContext : public AppComponentBase<OpenGLContext> {
public:
    //! Sets the window attributes before window creation.
    static void setupWindowAttributes();

    OpenGLContext();
    ~OpenGLContext();

    inline SDL_GLContext getContext() { return m_GLContext; }

private:
    SDL_GLContext m_GLContext = nullptr;

    static void gladPostCallback(const char *name, void *, int, ...);
    static const char *getGlErrorString(GLenum errorCode);
    static void APIENTRY debugMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                          GLsizei length, const GLchar *message,
                                          const void *userParam);
};

#endif
