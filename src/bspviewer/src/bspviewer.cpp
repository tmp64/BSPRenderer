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

static ConCommand cmd_getpos("getpos", "", [](const CmdString &) {
    auto pos = BSPViewer::get().getCameraPos();
    auto rot = BSPViewer::get().getCameraRot();
    printi("setpos {} {} {} {} {} {}", pos.x, pos.y, pos.z, rot.x, rot.y, rot.z);
});

static ConCommand cmd_setpos("setpos", "", [](const CmdString &cmd) {
    if (cmd.size() < 4) {
        printi("setpos <x> <y> <z> [pitch] [yaw] [roll]");
        return;
    }

    glm::vec3 pos = {std::stof(cmd[1]), std::stof(cmd[2]), std::stof(cmd[3])};
    BSPViewer::get().setCameraPos(pos);

    if (cmd.size() >= 7) {
        glm::vec3 rot = {std::stof(cmd[4]), std::stof(cmd[5]), std::stof(cmd[6])};
        BSPViewer::get().setCameraRot(rot);
    }
});

static ConVar<float> m_sens("m_sens", 0.15f, "Mouse sensitivity (degrees/pixel)");
static ConVar<float> cam_speed("cam_speed", 1000.f, "Camera speed");

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

    if (m_pLevelLoader) {
        // Level is loading
        AFW_ASSERT(!m_pWorldState);
        InputSystem::get().discardMouseMovement();
        loadingTick();
    } else if (m_pWorldState) {
        // Level is loaded
        processUserInput();
        MainViewRenderer::get().showMainView();
    } else {
        // Level is not loaded
        InputSystem::get().discardMouseMovement();
    }
}

void BSPViewer::drawBackground() {
    BaseClass::drawBackground();

    if (m_pWorldState) {
        MainViewRenderer::get().renderMainView();
    }
}

void BSPViewer::onWindowSizeChange(int wide, int tall) {
    m_flAspectRatio = (float)wide / tall;
    glViewport(0, 0, wide, tall);
}

void BSPViewer::processUserInput() {
    if (!InputSystem::get().isInputGrabbed()) {
        return;
    }

    // Rotate the camera
    {
        int xrel, yrel;
        InputSystem::get().getMouseMovement(xrel, yrel);
        glm::vec3 rot = m_vRot;

        rot.y -= xrel * m_sens.getValue();
        rot.x += yrel * m_sens.getValue();

        if (rot.x < -90.0f) {
            rot.x = -90.0f;
        } else if (rot.x > 90.0f) {
            rot.x = 90.0f;
        }

        if (rot.y < 0.0f) {
            rot.y = 360.0f + rot.y;
        } else if (rot.y > 360.f) {
            rot.y = rot.y - 360.0f;
        }

        m_vRot = rot;
    }

    // Translate the camera
    {
        float delta = cam_speed.getValue() * getTimeDelta();
        const Uint8 *state = SDL_GetKeyboardState(nullptr);

        auto fnMove = [&](float x, float y, float z) {
            m_vPos.x += x * delta;
            m_vPos.y += y * delta;
            m_vPos.z += z * delta;
        };

        // !!! In radians
        float pitch = glm::radians(m_vRot.x);
        float yaw = glm::radians(m_vRot.y);

        if (state[SDL_SCANCODE_LSHIFT]) {
            delta *= 5;
        }

        if (state[SDL_SCANCODE_W]) {
            fnMove(cos(yaw) * cos(pitch), sin(yaw) * cos(pitch), -sin(pitch));
        }

        if (state[SDL_SCANCODE_S]) {
            fnMove(-cos(yaw) * cos(pitch), -sin(yaw) * cos(pitch), sin(pitch));
        }

        if (state[SDL_SCANCODE_A]) {
            fnMove(-cos(yaw - glm::pi<float>() / 2), -sin(yaw - glm::pi<float>() / 2), 0);
        }

        if (state[SDL_SCANCODE_D]) {
            fnMove(cos(yaw - glm::pi<float>() / 2), sin(yaw - glm::pi<float>() / 2), 0);
        }
    }
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

            m_vPos = {0.f, 0.f, 0.f};
            m_vRot = {0.f, 0.f, 0.f};
        }
    } catch (const std::exception &e) {
        printe("Failed to load the map: {}", e.what());
        m_pLevelLoader = nullptr;
        m_pWorldState = nullptr;
    }
}
