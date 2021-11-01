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
#include <renderer/shader_manager.h>
#include <renderer/material_manager.h>

#include "bspviewer.h"

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
    MainViewRenderer::get().tick();

    if (m_pLevelLoader) {
        // Level is loading
        AFW_ASSERT(!m_pWorldState);
        loadingTick();
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
        }
    } catch (const std::exception &e) {
        printe("Failed to load the map: {}", e.what());
        m_pLevelLoader = nullptr;
        m_pWorldState = nullptr;
    }
}
