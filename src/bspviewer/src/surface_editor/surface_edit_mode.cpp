#include "surface_edit_mode.h"

SurfaceEditMode::SurfaceEditMode()
    : m_SelectTool(this) {
    registerTool(&m_SelectTool);
    activateTool(&m_SelectTool);
}

const char *SurfaceEditMode::getName() {
    return "Surface";
}
