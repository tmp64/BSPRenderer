#ifndef EDITOR_TOOL_H
#define EDITOR_TOOL_H
#include <appfw/utils.h>
#include <glm/glm.hpp>

class EditorMode;

class EditorTool : appfw::NoMove {
public:
    EditorTool(EditorMode *parentMode);
    virtual ~EditorTool();

    //! @returns the name of the tool, will be displayed in the list.
    virtual const char *getName() = 0;

    //! @returns whether the tool is active.
    inline bool isActive() { return m_bIsActive; }

    //! Called after level has been loaded.
    virtual void onLevelLoaded();

    //! Called when level has started unloading.
    virtual void onLevelUnloaded();

    //! Called when the tool is activated.
    virtual void onActivated();

    //! Called when the tool is deactivated.
    virtual void onDeactivated();

    //! Called when the main view is left-clicked.
    //! @param  position    Position of the click in pixels
    virtual void onMainViewClicked(const glm::vec2 &position);

    //! @returns whether to tick() when inactive.
    inline bool isAlwaysTickEnabled() { return m_bAlwaysTick; }

    //! Called every tick that the tool is active (or every tick if always tick enabled).
    virtual void tick();

protected:
    //! Enables/disables always tick
    virtual void setAlwaysTickEnabled(bool state);

private:
    EditorMode *m_pMode = nullptr;
    bool m_bIsActive = false;
    bool m_bAlwaysTick = false;
};

#endif
