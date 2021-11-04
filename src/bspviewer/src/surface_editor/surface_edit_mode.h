#ifndef SURFACE_EDITOR_SURFACE_EDIT_MODE_H
#define SURFACE_EDITOR_SURFACE_EDIT_MODE_H
#include "surface_select_tool.h"
#include "../editor_mode.h"

class SurfaceEditMode : public EditorMode {
public:
    SurfaceEditMode();
    const char *getName() override;
    void showInspector() override;

private:
    SurfaceSelectTool m_SelectTool;
};

#endif
