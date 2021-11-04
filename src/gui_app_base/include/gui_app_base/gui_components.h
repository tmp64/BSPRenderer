#ifndef GUI_APP_BASE_COMPONENTS_H
#define GUI_APP_BASE_COMPONENTS_H
#include "SDL.h"
#include <imgui.h>
#include <app_base/app_component.h>

//! Initializes SDL2
class SDLComponent : public AppComponentBase<SDLComponent> {
public:
    SDLComponent();
    ~SDLComponent();
};

//! The main app window
class MainWindowComponent : public AppComponentBase<MainWindowComponent> {
public:
    MainWindowComponent();
    ~MainWindowComponent();
    void lateInit() override;
    void saveWindowSize();
    void saveWindowPos();

    inline SDL_Window *getWindow() { return m_pWindow; }

private:
    SDL_Window *m_pWindow = nullptr;
};

//! ImGui
class ImGuiComponent : public AppComponentBase<ImGuiComponent> {
public:
    ImGuiComponent();
    void beginTick();
    void endTick();
    void render();

    //! Loads a font from path, size is scaled automatically.
    //! @returns font or nullptr on error
    ImFont *loadFont(std::string_view filepath, float sizePixels);

    //! Loads a font from path, size is scaled automatically.
    //! @returns font or the embedded font on error
    ImFont *loadFontOrDefault(std::string_view filepath, float sizePixels);

private:
    static constexpr float NORMAL_DPI = 96;

    float m_flScale = 0;
    void setupScale();
};

#endif
