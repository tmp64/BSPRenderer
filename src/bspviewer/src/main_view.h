#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H
#include <hlviewer/scene_view.h>

struct Ray;

class MainView : public SceneView {
public:
    static inline MainView *get() { return m_spInstance; }

	MainView(LevelAsset &level);
    ~MainView();

    //! @returns whether world surface are rendered
    bool isWorldRenderingEnabled();

    //! @returns whether entities are rendered
    bool isEntityRenderingEnabled();

    //! @returns whether to show trigger brushes.
    bool showTriggers();

    void showImage() override;

protected:
    void onMouseLeftClick(glm::ivec2 mousePos) override;

private:
	static inline MainView *m_spInstance = nullptr;
};

#endif
