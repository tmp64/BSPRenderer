#ifndef SURFACE_EDITOR_SURFACE_SELECT_TOOL_H
#define SURFACE_EDITOR_SURFACE_SELECT_TOOL_H
#include "../editor_tool.h"

class SurfaceSelectTool : public EditorTool {
public:
    //! Color of selection in gamma-space
    static constexpr glm::vec4 SELECTION_COLOR = glm::vec4(1, 0, 0, 0.25f);

    SurfaceSelectTool(EditorMode *parentMode);

    //! @returns the index of the selected surface or -1.
    inline int getSurfaceIndex() { return m_iSurface; }

    //! @returns the index of the selected entity or -1 if not an entity or no selection.
    inline int getEntityIndex() { return m_iEntity; }

    //! Clears the selection.
    void clearSelection();

    //! Sets the selection to specified surface and entity.
    void setSelection(int surfIdx, int entIdx);

    const char *getName() override;
    void onLevelUnloaded() override;
    void onActivated() override;
    void onDeactivated() override;
    void onMainViewClicked(const glm::vec2 &position) override;
    void tick() override;

private:
    int m_iSurface = -1;
    int m_iEntity = -1;

    void showTinting();
    void clearTinting();
};

#endif
