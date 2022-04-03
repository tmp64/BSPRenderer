#include "main_view.h"
#include "world_state.h"
#include "bspviewer.h"

extern ConVar<bool> r_drawworld;
extern ConVar<bool> r_drawents;

static ConVar<float> m_sens("m_sens", 0.15f, "Mouse sensitivity (degrees/pixel)");
static ConVar<float> cam_speed("cam_speed", 1000.f, "Camera speed");
static ConfigItem<float> v_fov("main_view_fov", 110.f, "Horizontal field of view");
static ConfigItem<bool> v_show_triggers("main_view_show_triggers", true, "Show triggers");
static KeyBind grabToggleBind("Toggle main view mouse grab", KeyCode::Z);

static ConCommand cmd_getpos("getpos", "", [](const CmdString &) {
    if (!MainView::get()) {
        printe("No map loaded.");
        return;
    }

    auto pos = MainView::get()->getCameraPos();
    auto rot = MainView::get()->getCameraRot();
    printi("setpos {} {} {} {} {} {}", pos.x, pos.y, pos.z, rot.x, rot.y, rot.z);
});

static ConCommand cmd_setpos("setpos", "", [](const CmdString &cmd) {
    if (cmd.size() < 4) {
        printi("setpos <x> <y> <z> [pitch] [yaw] [roll]");
        return;
    }

    if (!MainView::get()) {
        printe("No map loaded.");
        return;
    }

    glm::vec3 pos = {std::stof(cmd[1]), std::stof(cmd[2]), std::stof(cmd[3])};
    MainView::get()->setCameraPos(pos);

    if (cmd.size() >= 7) {
        glm::vec3 rot = {std::stof(cmd[4]), std::stof(cmd[5]), std::stof(cmd[6])};
        MainView::get()->setCameraRot(rot);
    }
});

MainView::MainView(LevelAsset &level)
    : SceneView(level) {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;
    setIgnoreWindowFocus(true);
}

MainView::~MainView() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
}

bool MainView::isWorldRenderingEnabled() {
    return r_drawworld.getValue();
}

bool MainView::isEntityRenderingEnabled() {
    return r_drawents.getValue();
}

bool MainView::showTriggers() {
    return v_show_triggers.getValue();
}

void MainView::showImage() {
    // FOV slider
    float fov = v_fov.getValue();
    ImGui::PushItemWidth(320);
    if (ImGui::SliderFloat("FOV", &fov, 90, 150, "%.f")) {
        v_fov.setValue(fov);
    }
    ImGui::PopItemWidth();

    // Show triggers checkbox
    bool bShowTriggers = showTriggers();
    ImGui::SameLine();
    if (ImGui::Checkbox("Show triggers", &bShowTriggers)) {
        v_show_triggers.setValue(bShowTriggers);
    }

    setFov(fov);
    setShowTriggers(bShowTriggers);
    SceneView::showImage();
}

void MainView::onMouseLeftClick(glm::ivec2 mousePos) {
    EditorMode *editor = BSPViewer::get().getActiveEditor();
    if (editor) {
        EditorTool *tool = editor->getActiveTool();
        if (tool) {
            tool->onMainViewClicked(mousePos);
        }
    }
}
