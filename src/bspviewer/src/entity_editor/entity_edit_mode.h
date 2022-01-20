#ifndef ENTITY_EDITOR_ENTITY_EDIT_MODE_H
#define ENTITY_EDITOR_ENTITY_EDIT_MODE_H
#include "editor_mode.h"
#include "entity_select_tool.h"

class EntityEditMode : public EditorMode {
public:
    EntityEditMode();
    const char *getName() override;
    void showInspector() override;

private:
    EntitySelectTool m_SelectTool;
};

#endif
