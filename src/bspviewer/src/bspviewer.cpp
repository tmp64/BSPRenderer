#include <ctime>
#include <iostream>
#include <fstream>
#include <appfw/init.h>
#include <appfw/appfw.h>
#include <appfw/compiler.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include "bspviewer.h"

extern ConVar<bool> dev_console;
extern ConVar<bool> prof_ui;
extern ConVar<bool> gpu_ui;
extern ConVar<bool> mat_ui;

static ConfigItem<bool> cfg_window_console("windows_console", false, "Window: Developer console",
                                           [](const bool &val) { dev_console.setValue(val); });
static ConfigItem<bool> cfg_window_prof("windows_profiler", false, "Window: Profiler",
                                        [](const bool &val) { prof_ui.setValue(val); });
static ConfigItem<bool> cfg_window_gpu("windows_gpu", false, "Window: GPU Stats",
                                       [](const bool &val) { gpu_ui.setValue(val); });
static ConfigItem<bool> cfg_window_mat("windows_mat", false, "Window: Material Stats",
                                       [](const bool &val) { mat_ui.setValue(val); });

static bool CfgMenuItem(const char *text, ConfigItem<bool> &cfg) {
    bool val = cfg.getValue();

    if (ImGui::MenuItem(text, nullptr, &val)) {
        cfg.setValue(val);
        return true;
    }

    return false;
}

static ConCommand cmd_map("map", "Loads a map", [](const CmdString &cmd) {
    if (cmd.size() != 2) {
        printi("Usage: map <map name>");
        return;
    }

    BSPViewer::get().loadLevel(cmd[1]);
});

//----------------------------------------------------------------

const AppInitInfo &app_getInitInfo() {
    static AppInitInfo info = {"bspviewer", fs::u8path("")};
    return info;
}

std::unique_ptr<AppBase> app_createSingleton() { return std::make_unique<BSPViewer>(); }

//----------------------------------------------------------------

BSPViewer::BSPViewer() {
    AFW_ASSERT(!m_sSingleton);
    m_sSingleton = this;

    setAutoClearEnabled(true);
    registerMode(&m_SurfaceEditor);
    registerMode(&m_EntityEditor);
    activateMode(&m_SurfaceEditor);
}

BSPViewer::~BSPViewer() {
    AFW_ASSERT(m_sSingleton);

    if (m_pLevelLoader) {
        // Abort loading process. This may block.
        printw("Level is still loading. Shutdown may take a while.");
        m_pLevelLoader = nullptr;
    }

    unloadLevel();
    m_sSingleton = nullptr;
}

void BSPViewer::beginTick() {
    BaseClass::beginTick();
    showDockSpace();
    showMainMenuBar();
}

void BSPViewer::tick() {
    BaseClass::tick();
    ImGui::ShowDemoWindow();
    showModeSelection();
    showToolSelection();
    MainViewRenderer::get().tick();
    showInspector();

    if (m_pLevelLoader) {
        // Level is loading
        AFW_ASSERT(!m_pWorldState);
        loadingTick();
    } else if (m_pWorldState) {
        tickModes();
    }
}

void BSPViewer::drawBackground() {
    BaseClass::drawBackground();

    if (m_pWorldState) {
        MainViewRenderer::get().renderMainView();
    }
}

void BSPViewer::onWindowSizeChange(int wide, int tall) {
    glViewport(0, 0, wide, tall);
}

void BSPViewer::loadLevel(const std::string &name) {
    if (m_pLevelLoader) {
        printe("Can't load a level while loading another level.");
        return;
    }

    unloadLevel();

    std::string path = "assets:maps/" + name + ".bsp";    
    printi("Loading map {}", path);

    try {
        m_MapName = name;
        m_pLevelLoader = std::make_unique<LevelLoader>(path);
    } catch (const std::exception &e) {
        printe("Failed to load the map: {}", e.what());
        return;
    }
}

void BSPViewer::unloadLevel() {
    if (m_pLevelLoader) {
        // Level loading can't be aborted
        // Can't block until it finished either: event loop must be running for asset loading to
        // finish
        AFW_ASSERT(!m_pWorldState);
        throw std::logic_error("can't unload while loading");
    }

    if (m_pWorldState) {
        for (EditorMode *mode : m_EditorModes) {
            mode->onLevelUnloaded();
        }

        m_pWorldState = nullptr;
        return;
    }
}

void BSPViewer::showDockSpace() {
    ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), flags);
}

void BSPViewer::showMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Quit")) {
                quit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {
            } // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {
            }
            if (ImGui::MenuItem("Copy", "CTRL+C")) {
            }
            if (ImGui::MenuItem("Paste", "CTRL+V")) {
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows")) {
            CfgMenuItem("Developer console", cfg_window_console);
            CfgMenuItem("Profiler", cfg_window_prof);
            CfgMenuItem("GPU stats", cfg_window_gpu);
            CfgMenuItem("Maerial stats", cfg_window_mat);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void BSPViewer::loadingTick() {
    try {
        if (m_pLevelLoader->tick()) {
            // Loading has finished
            LevelAssetRef level = m_pLevelLoader->getLevel();
            m_pLevelLoader = nullptr;
            m_pWorldState = std::make_unique<WorldState>(level);

            for (EditorMode *mode : m_EditorModes) {
                mode->onLevelLoaded();
            }
        }
    } catch (const std::exception &e) {
        printe("Failed to load the map: {}", e.what());
        m_pLevelLoader = nullptr;
        m_pWorldState = nullptr;
    }
}

void BSPViewer::registerMode(EditorMode *mode) {
    m_EditorModes.push_back(mode);
}

void BSPViewer::activateMode(EditorMode *mode) {
    if (m_pActiveMode) {
        m_pActiveMode->onDeactivated();
    }

    m_pActiveMode = mode;

    if (m_pActiveMode) {
        mode->onActivated();
    }
}

void BSPViewer::tickModes() {
    for (EditorMode *mode : m_EditorModes) {
        AFW_ASSERT(mode->isActive() ? mode == m_pActiveMode : mode != m_pActiveMode);
        if (mode->isActive() || mode->isAlwaysTickEnabled()) {
            mode->tick();
        }
    }
}

void BSPViewer::showModeSelection() {
    if (ImGui::Begin("Mode Selection")) {
        for (size_t i = 0; i < m_EditorModes.size(); i++) {
            if (i > 0)
                ImGui::SameLine();
            if (ImGui::RadioButton(m_EditorModes[i]->getName(),
                                   m_EditorModes[i] == m_pActiveMode)) {
                activateMode(m_EditorModes[i]);
            }
        }
    }

    ImGui::End();
}

void BSPViewer::showToolSelection() {
    if (ImGui::Begin("Tool Selection")) {
        if (m_pActiveMode) {
            m_pActiveMode->showToolSelection();
        }
    }

    ImGui::End();
}

void BSPViewer::showInspector() {
    if (ImGui::Begin("Inspector")) {
        if (m_pWorldState && m_pActiveMode) {
            m_pActiveMode->showInspector();
        }
    }

    ImGui::End();
}
