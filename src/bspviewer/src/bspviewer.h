#ifndef APP_H
#define APP_H
#include <gui_app/gui_app_base.h>
#include <renderer/scene_renderer.h>

class BSPViewer : public GuiAppBase {
public:
    static BSPViewer &get();

    BSPViewer();
    ~BSPViewer();

    virtual void tick() override;
    virtual void draw() override;
    virtual void onWindowSizeChange(int wide, int tall) override;

    /**
     * Load a .bsp map
     * @param   name    Name of the map, without maps/ and .bsp.
     */
    void loadMap(const std::string &name);

    /**
     * Shows dialog with framerates and position.
     */
    void showInfoDialog();

    /**
     * Moves and rotates the camera if input is grabbed.
     */
    void processUserInput();

    void setDrawDebugTextEnabled(bool state);
    inline bool isDrawDebugTextEnabled() { return m_bDrawDebugText; }
    inline glm::vec3 getCameraPos() { return m_vPos; }

private:
    bsp::Level m_LoadedLevel;
    SceneRenderer m_Renderer;
    glm::vec3 m_vPos = {0.f, 0.f, 0.f};
    glm::vec3 m_vRot = {0.f, 0.f, 0.f};
    float m_flAspectRatio = 1.f;

    bool m_bDrawDebugText = true;

    static inline BSPViewer *m_sSingleton = nullptr;
};

inline BSPViewer &BSPViewer::get() { return *m_sSingleton; }

using namespace std::literals::string_literals;

#endif
