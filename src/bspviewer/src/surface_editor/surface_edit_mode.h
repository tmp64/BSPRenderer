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
    void onLevelUnloaded() override;
    void showInspector() override;

private:
    class SurfaceData {
    public:
        SurfaceData(SurfaceEditMode *editor, int surfIdx);
        ~SurfaceData();
        inline int getIdx() { return m_iIdx; } //!< @returns the index or -1
        void saveChanges();
        void showSurfaceProps();
        void showMapMatProps();
        void showWadMatProps();
        void showBaseMatProps();

    private:
        SurfaceEditMode *m_pEditor = nullptr;
        int m_iIdx = -1;
        Material *m_pMaterial = nullptr;
        YAML::Node m_SurfaceYaml;
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
        std::string getSurfaceYamlPath();
        void loadMaterial(const std::string &vpath, MaterialPropsFile &mat, MaterialPropsFile &origMat);
        void loadBaseMaterial(bool silenceNotFound);
        void loadSurfaceProps();

        template <typename T, bool bAllowRemove>
        void saveFile(const std::string &vpath, T &data, T &origData);
        void saveSurfaceProps();
    };

    SurfaceSelectTool m_SelectTool;
    std::unique_ptr<SurfaceData> m_pSurfaceData;
};

#endif
