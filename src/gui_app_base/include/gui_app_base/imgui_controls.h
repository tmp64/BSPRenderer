#ifndef IMGUI_CONTROLS_H
#define IMGUI_CONTROLS_H
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <appfw/appfw.h>
#include <imgui.h>

// TODO: Move all that into a .cpp file

inline const char *VectorToString(const glm::vec3 &vec, int width = 5, int prec = 2) {
	static char buf[128];

    snprintf(buf, sizeof(buf), "[%*.*f, %*.*f, %*.*f]",
		width, prec, vec.x,
		width, prec, vec.y,
		width, prec, vec.z
	);

	return buf;
}

inline const char *QuatToString(const glm::quat &quat, int width = 5, int prec = 2) {
	static char buf[128];

    snprintf(buf, sizeof(buf), "[%*.*f, %*.*f, %*.*f, %*.*f]",
		width, prec, quat.x,
		width, prec, quat.y,
		width, prec, quat.z,
		width, prec, quat.w
	);

	return buf;
}

inline void CvarCheckbox(const char *text, ConVar<bool> &cvar) {
    bool value = cvar.getValue();

	if (ImGui::Checkbox(text, &value)) {
        cvar.setValue(value);
	}
}

inline void CvarSlider(const char *label, ConVar<float> &cvar, float v_min, float v_max) {
    float val = cvar.getValue();

    if (ImGui::SliderFloat(label, &val, v_min, v_max)) {
        cvar.setValue(val);
	}
}

//! Shows an ImGui frame with close button that sets a cvar to false.
inline bool ImGuiBeginCvar(const char *name, ConVar<bool> &cvar, ImGuiWindowFlags flags = 0) {
    bool isVisible = cvar.getValue();
    bool result = ImGui::Begin(name, &isVisible, flags);

    if (isVisible != cvar.getValue()) {
        cvar.setValue(isVisible);
    }

    return result;
}

#endif
