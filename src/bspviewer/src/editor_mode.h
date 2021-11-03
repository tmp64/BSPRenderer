#ifndef EDITOR_MODE_H
#define EDITOR_MODE_H
#include <appfw/utils.h>

class EditorTool;

class EditorMode : appfw::NoMove {
public:
    EditorMode();
    virtual ~EditorMode();

    //! @returns the name of the mode, will be displayed in the list.
    virtual const char *getName() = 0;

    //! @returns whether the mode is active.
    inline bool isActive() { return m_bIsActive; }

    //! @returns the list of all tools.
    inline std::vector<EditorTool *> getToolList() { return m_Tools; }

    //! @returns the active tool or nullptr.
    inline EditorTool *getActiveTool() { return m_pActiveTool; }

    //! Called after level has been loaded.
    virtual void onLevelLoaded();

    //! Called when level has started unloading.
    virtual void onLevelUnloaded();

    //! Called when the mode is activated.
    virtual void onActivated();

    //! Called when the mode is deactivated.
    virtual void onDeactivated();

    //! @returns whether to tick() when inactive.
    inline bool isAlwaysTickEnabled() { return m_bAlwaysTick; }

    //! Called every tick that the mode is active (or every tick if always tick enabled).
    virtual void tick();

    //! Called to show the list of tools. Called from inside ImGui window context.
    virtual void showToolSelection();

protected:
    //! Enables/disables always tick
    virtual void setAlwaysTickEnabled(bool state);

    //! Adds a tool to the list.
    //! @param  tool    Non-owning pointer to the tool.
    void registerTool(EditorTool *tool);

    //! Runs tick() on the tools that need it. Remember to call this from tick().
    void tickTools();

    //! Activates a specific tool or, if nullptr, no tool.
    virtual void activateTool(EditorTool *tool);

private:
    bool m_bIsActive = false;
    bool m_bAlwaysTick = false;
    std::vector<EditorTool *> m_Tools;
    EditorTool *m_pActiveTool = nullptr;
};

#endif
