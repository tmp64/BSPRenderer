#ifndef ENTITY_EDITOR_ENTITY_SELECT_TOOL_H
#define ENTITY_EDITOR_ENTITY_SELECT_TOOL_H
#include "editor_tool.h"

class EntitySelectTool : public EditorTool {
public:
    //! Color of selection in gamma-space
    static constexpr glm::vec4 SELECTION_COLOR = glm::vec4(1, 0, 0, 0.7f);

    EntitySelectTool(EditorMode *parentMode);

    //! @returns the index of the selected entity or -1 if not an entity or no selection.
    inline int getEntityIndex() { return m_iEntIdx; }

    //! Clears the selection.
    void clearSelection();

    //! Sets the selection to specified surface and entity.
    void setSelection(int entIdx);

    const char *getName() override;
    void onLevelUnloaded() override;
    void onActivated() override;
    void onDeactivated() override;
    void onMainViewClicked(const glm::vec2 &position) override;
    void tick() override;

private:
    int m_iEntIdx = -1;

    void showTinting();
    void clearTinting();
};

#endif
