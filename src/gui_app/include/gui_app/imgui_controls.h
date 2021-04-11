#ifndef IMGUI_CONTROLS_H
#define IMGUI_CONTROLS_H
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <appfw/services.h>
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

#endif
