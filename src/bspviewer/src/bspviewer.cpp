#include <ctime>
#include <iostream>
#include <fstream>
#include <appfw/init.h>
#include <appfw/services.h>
#include <appfw/compiler.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <renderer/shader_manager.h>
#include <renderer/material_manager.h>
#include <renderer/frame_console.h>

#include "bspviewer.h"
#include "demo.h"
#include "bsp_tree.h"

static ConCommand quit_cmd("quit", "Exits the app", [](auto &) { BSPViewer::get().quit(); });

static ConCommand cmd_map("map", "Loads a map", [](const appfw::ParsedCommand &cmd) {
    if (cmd.size() != 2) {
        logInfo("Usage: map <map name>");
        return;
    }

    BSPViewer::get().loadMap(cmd[1]);
});

static ConCommand cmd_toggle_debug_text("toggle_debug_text", "", [](const appfw::ParsedCommand &) {
    BSPViewer::get().setDrawDebugTextEnabled(!BSPViewer::get().isDrawDebugTextEnabled());
});

static ConVar<float> m_sens("m_sens", 0.15f, "Mouse sensitivity (degrees/pixel)");
static ConVar<float> cam_speed("cam_speed", 1000.f, "Camera speed");
static ConVar<float> fov("fov", 120.f, "Horizontal field of view");
static ConVar<bool> gl_break_on_error("gl_break_on_error", false, "Break in debugger on OpenGL error");

//----------------------------------------------------------------

/**
 * Returns info for current app's initialization.
 * Must be implemented by the app.
 */
const GuiAppInfo &app_getInitInfo() {
    static GuiAppInfo info = {"bspviewer", fs::u8path("")};

    return info;
}

/**
 * Creates an instance of GuiAppBase.
 * Must be implemented by the app.
 */
std::unique_ptr<GuiAppBase> app_createSingleton() { return std::make_unique<BSPViewer>(); }

/**
 * SDL2 application entry point.
 */
extern "C" int main(int argc, char **argv) { return app_runmain(argc, argv); }

//----------------------------------------------------------------

BSPViewer::BSPViewer() {
    AFW_ASSERT(!m_sSingleton);
    m_sSingleton = this;

    setAutoClearEnabled(true);
    setAutoClearColor({0, 0.5f, 0, 1});

    // Init renderer
    MaterialManager::get().addWadFile(getFileSystem().findFile("halflife.wad", "assets"));

    // Init input
    InputSystem::get().bindKey(SDL_SCANCODE_F3, "toggle_debug_text");
    InputSystem::get().bindKey(SDL_SCANCODE_GRAVE, "toggleconsole");
    InputSystem::get().bindKey(InputSystem::get().getScancodeForKey("mouse1"), "trace1");
    InputSystem::get().bindKey(InputSystem::get().getScancodeForKey("mouse2"), "trace2");

    loadMap("crossfire");
}

BSPViewer::~BSPViewer() {
    AFW_ASSERT(m_sSingleton);

    // Shutdown renderer
    m_Renderer.setLevel(nullptr, "");

    m_sSingleton = nullptr;
}

void BSPViewer::tick() {
    //
    ImGui::ShowDemoWindow();
    //

    processUserInput();
    showInfoDialog();
}

void BSPViewer::draw() {
    m_Renderer.setPerspective(fov.getValue(), m_flAspectRatio, 4, 4096);
    m_Renderer.setPerspViewOrigin(m_vPos, m_vRot);
    m_Renderer.renderScene(0);
}

void BSPViewer::onWindowSizeChange(int wide, int tall) {
    m_flAspectRatio = (float)wide / tall;
    glViewport(0, 0, wide, tall);
    m_Renderer.setViewportSize({wide, tall});
}

void BSPViewer::showInfoDialog() {
    static bool open = true;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowBgAlpha(0.2f);

    ImColor cyan =
        ImColor(FrameConsole::Cyan.r(), FrameConsole::Cyan.g(), FrameConsole::Cyan.b(), FrameConsole::Cyan.a());
    ImColor red = ImColor(FrameConsole::Red.r(), FrameConsole::Red.g(), FrameConsole::Red.b(), FrameConsole::Red.a());

    if (ImGui::Begin("BSPViewer Stats", nullptr, window_flags)) {
        showStatsUI();

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
        float delta = cam_speed.getValue() * (float)m_flLastFrameTime;
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

void BSPViewer::loadMap(const std::string &name) {
    m_Renderer.setLevel(nullptr, "");

    std::string path = "maps/" + name + ".bsp";    
    logInfo("Loading map {}", path);

    try {
        fs::path bspPath = getFileSystem().findFile(path, "assets");
        m_LoadedLevel.loadFromFile(bspPath);
        m_Renderer.setLevel(&m_LoadedLevel, bspPath);

        g_BSPTree.setLevel(&m_LoadedLevel);
        g_BSPTree.createTree();

        m_vPos = {0.f, 0.f, 0.f};
        m_vRot = {0.f, 0.f, 0.f};

        logInfo("Map loaded");
    } catch (const std::exception &e) {
        logError("Failed to load map: {}", e.what());
        return;
    }
}
