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

static ConCommand cmd_toggle_debug_text("toggle_debug_text", "", []() {
    BSPViewer::get().setDrawDebugTextEnabled(!BSPViewer::get().isDrawDebugTextEnabled());
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
    setAutoClearColor({0, 0.5f, 0, 1});

    // Init input
    InputSystem::get().bindKey(SDL_SCANCODE_F3, "toggle_debug_text");
    InputSystem::get().bindKey(SDL_SCANCODE_GRAVE, "toggleconsole");
    InputSystem::get().bindKey(InputSystem::get().getScancodeForKey("mouse1"), "trace1");
    InputSystem::get().bindKey(InputSystem::get().getScancodeForKey("mouse2"), "trace2");

    loadLevel("crossfire");
}

BSPViewer::~BSPViewer() {
    AFW_ASSERT(m_sSingleton);

    while (m_LoadingState == LoadingState::Loading) {
        // Run until loading finishes
        loadingTick();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    unloadLevel();
    m_sSingleton = nullptr;
}

void BSPViewer::tick() {
    BaseClass::tick();
    ImGui::ShowDemoWindow();

    switch (m_LoadingState) {
    case LoadingState::NotLoaded: {
        InputSystem::get().discardMouseMovement();
        break;
    }
    case LoadingState::Loading: {
        loadingTick();
        InputSystem::get().discardMouseMovement();
        break;
    }
    case LoadingState::Loaded: {
        Renderer::get().tick();
        processUserInput();
        break;
    }
    }

    showInfoDialog();
}

void BSPViewer::drawBackground() {
    BaseClass::drawBackground();

    if (m_LoadingState == LoadingState::Loaded) {
        Renderer::get().draw();
    }
}

void BSPViewer::onWindowSizeChange(int wide, int tall) {
    m_flAspectRatio = (float)wide / tall;
    glViewport(0, 0, wide, tall);
    Renderer::get().setViewportSize({wide, tall});
}

void BSPViewer::showInfoDialog() {
    static bool open = true;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowBgAlpha(0.2f);

    ImColor cyan = ImColor(0, 255, 255, 255);
    ImColor red = ImColor(255, 0, 0, 255);

    if (ImGui::Begin("BSPViewer Stats", nullptr, window_flags)) {
        ImGui::Separator();
        ImGui::Text("Pos: X: %9.3f / Y: %9.3f / Z: %9.3f", m_vPos.x, m_vPos.y, m_vPos.z);
        ImGui::Text("Rot: P: %9.3f / Y: %9.3f / R: %9.3f", m_vRot.x, m_vRot.y, m_vRot.z);

        ImGui::Separator();
        ImGui::Text("%s", glGetString(GL_VENDOR));
        ImGui::Text("%s", glGetString(GL_RENDERER));

        ImGui::Separator();
        if (InputSystem::get().isInputGrabbed()) {
            ImGui::Text("Input grabbed");
        } else {
            ImGui::TextColored(red, "Input released");
        }
    }
    ImGui::End();
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

void BSPViewer::setDrawDebugTextEnabled(bool state) { m_bDrawDebugText = state; }

void BSPViewer::loadLevel(const std::string &name) {
    if (m_LoadingState == LoadingState::Loading) {
        printe("Can't load a level while loading another level.");
        return;
    }

    unloadLevel();

    std::string path = "assets:maps/" + name + ".bsp";    
    printi("Loading map {}", path);

    try {
        AFW_ASSERT(!m_pWorldState);
        fs::path bspPath = getFileSystem().findExistingFile(path);
        m_LoadedLevel.loadFromFile(bspPath);
        m_pWorldState = new WorldState();
        WorldState::get().loadLevel(m_LoadedLevel);
        Renderer::get().loadLevel(path);

        m_vPos = {0.f, 0.f, 0.f};
        m_vRot = {0.f, 0.f, 0.f};

        m_LoadingState = LoadingState::Loading;
    } catch (const std::exception &e) {
        printe("Failed to load the map: {}", e.what());
        unloadLevel();
        return;
    }
}

void BSPViewer::unloadLevel() {
    if (m_LoadingState == LoadingState::NotLoaded) {
        return;
    }

    AFW_ASSERT(m_LoadingState != LoadingState::Loading);

    if (m_LoadingState == LoadingState::Loading) {
        throw std::logic_error("can't unload while loading");
    }

    delete m_pWorldState;
    m_pWorldState = nullptr;
    Renderer::get().unloadLevel();
    m_LoadingState = LoadingState::NotLoaded;
}

void BSPViewer::loadingTick() {
    try {
        if (Renderer::get().loadingTick()) {
            printi("Loading finished");
            m_LoadingState = LoadingState::Loaded;
        }
    } catch (const std::exception &e) {
        printe("Failed to load the map: {}", e.what());
        m_LoadingState = LoadingState::Loaded;
        unloadLevel();
    }
}
