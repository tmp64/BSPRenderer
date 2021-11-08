#ifndef SURFACE_EDITOR_SURFACE_EDIT_MODE_H
#define SURFACE_EDITOR_SURFACE_EDIT_MODE_H
#include "surface_select_tool.h"
#include "files.h"
#include "../editor_mode.h"

class Material;

class SurfaceEditMode : public EditorMode {
public:
    SurfaceEditMode();
    const char *getName() override;
    void showInspector() override;

private:
    class SurfaceData {
    public:
        SurfaceData();
        inline int getIdx() { return m_iIdx; } //!< @returns the index or -1
        void load(SurfaceEditMode *editor, int surfIdx);
        void saveChanges();
        void showSurfaceProps();
        void showMapMatProps();
        void showWadMatProps();
        void showBaseMatProps();

    private:
        SurfaceEditMode *m_pEditor = nullptr;
        int m_iIdx = -1;
        Material *m_pMaterial = nullptr;
        bool m_bCurrentBaseNotFound = false;

        SurfacePropsFile m_Surf;
        MaterialPropsFile m_Map;
        MaterialPropsFile m_Wad;
        BaseMaterialPropsFile m_Base;

        SurfacePropsFile m_OrigSurf;
        MaterialPropsFile m_OrigMap;
        MaterialPropsFile m_OrigWad;
        BaseMaterialPropsFile m_OrigBase;

        void showMatProps(MaterialPropsFile &mat);

        std::string getBaseMaterialName();
        void loadBaseMaterial(bool silenceNotFound);

        template <typename T, bool bAllowRemove>
        void saveFile(const std::string &vpath, T &data, T &origData);
    };

    SurfaceSelectTool m_SelectTool;
    SurfaceData m_SurfaceData;
};

#endif
