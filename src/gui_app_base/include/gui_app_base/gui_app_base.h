#ifndef GUI_APP_GUI_APP_BASE_H
#define GUI_APP_GUI_APP_BASE_H
#include <glm/glm.hpp>
#include <appfw/appfw.h>
#include <app_base/app_base.h>
#include <gui_app_base/gui_components.h>
#include <gui_app_base/opengl_context.h>
#include <input/input_system.h>
#include <input/key_bind.h>
#include <renderer/gpu_resource_manager.h>
#include <renderer/shader_manager.h>
#include <renderer/material_manager.h>

class DevConsoleDialog;
class ProfilerDialog;

//! Base class for apps that use OpenGL and ImGui
class GuiAppBase : public AppBase {
public:
    //! Returns the instance of GuiAppBase.
    static inline GuiAppBase &getBaseInstance() { return static_cast<GuiAppBase &>(AppBase::getBaseInstance()); }

    GuiAppBase();
    ~GuiAppBase();

    //! Whether automatically call glClear.
    inline bool isAutoClearEnabled() { return m_bAutoClear; }

    //! Enable/disable autoclear.
    inline void setAutoClearEnabled(bool state) { m_bAutoClear = state; }

    //! Sets autoclear color.
    inline void setAutoClearColor(glm::vec4 color) { m_vAutoClearColor = color; }

protected:
    void lateInit() override;
    void beginTick() override;
    void tick() override;
    void endTick() override;

    //! Updates screen contents
    virtual void refreshScreen();

    //! Draws the background image (e.g. clears the color buffer)
    virtual void drawBackground();

    //! Draws the foreground image (e.g. ImGui)
    virtual void drawForeground();

    //! Handles an SDL2 event
    //! @returns true if event was processed
    virtual bool handleSDLEvent(const SDL_Event &event);
    
    //! Called when window size is changed
    virtual void onWindowSizeChange(int wide, int tall);

private:
    struct GraphicsSubSystem {
        GPUResourceManager m_GPUManager;
        ShaderManager m_ShaderManager;
        MaterialManager m_MaterialManager;
    };

    SDLComponent m_SDL;
    MainWindowComponent m_MainWindow;
    OpenGLContext m_GLContext;
    GraphicsSubSystem m_Graphics;
    ImGuiComponent m_ImGui;
    InputSystem m_InputSystem;
    std::unique_ptr<DevConsoleDialog> m_pDevConsole;
    std::unique_ptr<ProfilerDialog> m_pProfilerUI;

    glm::ivec2 m_vWindowSize = glm::ivec2(0, 0);

    bool m_bAutoClear = true;
    glm::vec4 m_vAutoClearColor = glm::vec4(0x0D / 255.0f, 0x27 / 255.0f, 0x40 / 255.0f, 1);

    //! Fully processes the SDL event queue
    void pollEvents();
};

#endif
