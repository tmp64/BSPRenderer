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

KeyBind key_switch_modes("Switch modes", KeyCode::F);

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
    handleSwitchModesKey();
    showMainView();
    showInspector();

    if (m_pLevelLoader) {
        // Level is loading
        loadingTick();
    } else if (m_pLevelWorldState) {
        m_pLevelWorldState->tick(getTimeDelta());
        tickModes();
    }
}

void BSPViewer::drawBackground() {
    BaseClass::drawBackground();

    if (MainView::get()) {
        MainView::get()->renderBackBuffer();
    }
}

void BSPViewer::onWindowSizeChange(int wide, int tall) {
    GuiAppBase::onWindowSizeChange(wide, tall);
    glViewport(0, 0, wide, tall);
}

void BSPViewer::loadLevel(const std::string &name) {
    unloadLevel();

    std::string path = "assets:maps/" + name + ".bsp";    
    printi("Loading map {}", path);
    m_bLevelLoadFailed = false;

    try {
        m_MapName = name;
        m_pLevelLoader = std::make_unique<AssetLoader>();
        m_Level = m_pLevelLoader->loadLevel(path);
    } catch (const std::exception &e) {
        unloadLevel();
        m_bLevelLoadFailed = true;
        m_LevelLoadError = e.what();
        printe("Failed to load the map: {}", m_LevelLoadError);
    }
}

void BSPViewer::unloadLevel() {
    if (m_pLevelLoader) {
        m_pLevelLoader = nullptr;
        m_Level = LevelAsset();
    } else if (m_pLevelWorldState) {
        AFW_ASSERT(m_pLevelMainView);

        for (EditorMode *mode : m_EditorModes) {
            mode->onLevelUnloaded();
        }

        m_pLevelMainView->setWorldState(nullptr);
        m_pLevelWorldState = nullptr;
        m_pLevelMainView = nullptr;
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
        m_pLevelLoader->processQueue();

        if (m_pLevelLoader->isFinished()) {
            onLevelLoadingFinished();
        }
    } catch (const std::exception &e) {
        m_bLevelLoadFailed = true;
        m_LevelLoadError = e.what();
        printe("Failed to load the map: {}", m_LevelLoadError);
    }
}

void BSPViewer::onLevelLoadingFinished() {
    appfw::Timer timer;
    m_pLevelLoader = nullptr;
    m_pLevelMainView = std::make_unique<MainView>(m_Level);
    m_pLevelWorldState = std::make_unique<WorldState>(m_Level.getLevel(), m_pLevelMainView.get());
    m_pLevelMainView->setWorldState(m_pLevelWorldState.get());

    for (EditorMode *mode : m_EditorModes) {
        mode->onLevelLoaded();
    }

    printi("Finished loading in {:.3f} ms", timer.dseconds() * 1000.0);
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

void BSPViewer::handleSwitchModesKey() {
    if (key_switch_modes.isPressed()) {
        // Find current mode
        size_t curMode = 0;
        for (size_t i = 0; i < m_EditorModes.size(); i++) {
            if (m_pActiveMode == m_EditorModes[i]) {
                curMode = i;
                break;
            }
        }

        // Switch to the next one
        size_t newMode = (curMode + 1) % m_EditorModes.size();
        activateMode(m_EditorModes[newMode]);
    }
}

void BSPViewer::showMainView() {
    if (!ImGui::Begin("Scene")) {
        ImGui::End();
    }

    if (m_bLevelLoadFailed) {
        ImGui::TextUnformatted("Map failed to load:");
        ImGui::TextUnformatted(m_LevelLoadError.c_str());
    } else if (m_pLevelLoader) {
        int finishedCount = m_pLevelLoader->getFinishedItemCount();
        int totalCount = m_pLevelLoader->getTotalItemCount();
        float percent = totalCount == 0 ? 0.0f : (float)finishedCount / totalCount;
        ImGui::Text("Loading %.f (%d/%d): %s", percent * 100, finishedCount, totalCount,
                    m_pLevelLoader->getLastItemName().c_str());
        ImGui::ProgressBar(percent);
    } else if (m_pLevelWorldState) {
        AFW_ASSERT(m_pLevelMainView);
        m_pLevelMainView->showImage();
    }

    ImGui::End();
}

void BSPViewer::showInspector() {
    if (ImGui::Begin("Inspector")) {
        if (m_pLevelWorldState && m_pActiveMode) {
            m_pActiveMode->showInspector();
        }
    }

    ImGui::End();
}
