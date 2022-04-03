#include <imgui.h>
#include "surface_edit_mode.h"
#include "../world_state.h"
#include "../main_view.h"
#include "../bspviewer.h"

static constexpr float TEXTURE_PREVIEW_SIZE = 96.0f;

SurfaceEditMode::SurfaceData::SurfaceData(SurfaceEditMode *editor, int surfIdx)
    : m_Wad(true)
    , m_OrigWad(true)
    , m_Map(false)
    , m_OrigMap(false) {
    m_pEditor = editor;
    m_iIdx = surfIdx;

    auto &matload = WorldState::get()->getMaterialLoader();
    Material *material = m_pMaterial = MainView::get()->getSurfaceMaterial(surfIdx);

    loadMaterial(matload.getWadYamlPath(material->getName(), material->getWadName()), m_Wad,
                 m_OrigWad);
    loadBaseMaterial(false);
    loadMaterial(matload.getMapYamlPath(material->getName()), m_Map, m_OrigMap);
    loadSurfaceProps();
}

SurfaceEditMode::SurfaceData::~SurfaceData() {
    saveChanges();
}

void SurfaceEditMode::SurfaceData::saveChanges() {
    auto &matload = WorldState::get()->getMaterialLoader();
    saveFile<MaterialPropsFile, true>(matload.getMapYamlPath(m_pMaterial->getName()), m_Map,
                                      m_OrigMap);
    saveFile<MaterialPropsFile, true>(
        matload.getWadYamlPath(m_pMaterial->getName(), m_pMaterial->getWadName()), m_Wad,
        m_OrigWad);
    saveFile<BaseMaterialPropsFile, false>(matload.getBaseYamlPath(getBaseMaterialName()), m_Base,
                                           m_OrigBase);
    saveSurfaceProps();
}

void SurfaceEditMode::SurfaceData::showSurfaceProps() {
    ImGui::DragFloat("Lightmap scale", &m_Surf.flLightmapScale, 0.02f, 0.5f, 2.0f);
    ImGui::DragFloat("L. Int. scale", &m_Surf.flLightIntensityScale, 0.005f);

    ImGui::Checkbox("Light", &m_Surf.bOverrideLight);
    ImGui::BeginDisabled(!m_Surf.bOverrideLight);
    ImGui::ColorEdit3("Color", glm::value_ptr(m_Surf.vLightColor));
    ImGui::InputFloat("Intensity", &m_Surf.flLightIntensity);
    ImGui::EndDisabled();

    ImGui::Checkbox("Reflectivity", &m_Surf.bHasReflectivity);
    ImGui::BeginDisabled(!m_Surf.bHasReflectivity);
    ImGui::InputFloat("##refl", &m_Surf.flReflectivity);
    ImGui::EndDisabled();
}

void SurfaceEditMode::SurfaceData::showMapMatProps() {
    showMatProps(m_Map);
}

void SurfaceEditMode::SurfaceData::showWadMatProps() {
    showMatProps(m_Wad);
}

void SurfaceEditMode::SurfaceData::showBaseMatProps() {
    ImGui::Text("Name: %s", m_Base.szName);
    ImGui::InputFloat("Reflectivity", &m_Base.flReflectivity);
}

void SurfaceEditMode::SurfaceData::showMatProps(MaterialPropsFile &mat) {
    ImGui::DragFloat("L. Int. scale", &mat.flLightIntensityScale, 0.005f);

    ImGui::Checkbox("Light", &mat.bOverrideLight);
    ImGui::BeginDisabled(!mat.bOverrideLight);
    ImGui::ColorEdit3("Color", glm::value_ptr(mat.vLightColor));
    ImGui::InputFloat("Intensity", &mat.flLightIntensity);
    ImGui::EndDisabled();

    ImGui::Checkbox("Reflectivity", &mat.bHasReflectivity);
    ImGui::BeginDisabled(!mat.bHasReflectivity);
    ImGui::InputFloat("##refl", &mat.flReflectivity);
    ImGui::EndDisabled();

    if (&mat == &m_Wad) {
        if (ImGui::InputText("Base material", mat.szBaseName, sizeof(mat.szBaseName))) {
            loadBaseMaterial(true);
        }

        if (m_bCurrentBaseNotFound) {
            ImGui::TextColored(ImColor(255, 0, 0), "File not found");
        }
    }
}

std::string SurfaceEditMode::SurfaceData::getBaseMaterialName() {
    auto &matload = WorldState::get()->getMaterialLoader();
    std::string baseMatName = m_Wad.szBaseName;

    if (baseMatName.empty()) {
        SoundMaterials::Type type = matload.getSoundMaterials().findMaterial(m_pMaterial->getName());
        baseMatName = matload.getSoundMaterials().getTypeName(type);
    }

    return baseMatName;
}

std::string SurfaceEditMode::SurfaceData::getSurfaceYamlPath() {
    return fmt::format("assets:mapsrc/{}.rad.surf.yaml", BSPViewer::get().getMapName());
}

void SurfaceEditMode::SurfaceData::loadMaterial(const std::string &vpath, MaterialPropsFile &mat,
                                                MaterialPropsFile &origMat) {
    try {
        fs::path path = getFileSystem().findExistingFile(vpath, std::nothrow);

        if (!path.empty()) {
            std::ifstream file(path);
            YAML::Node yaml = YAML::Load(file);
            origMat.loadFromYaml(yaml);
            mat = origMat;
        }
    } catch (const std::exception &e) {
        printe("Failed to load {}: {}", vpath, e.what());
    }
}

void SurfaceEditMode::SurfaceData::loadBaseMaterial(bool silenceNotFound) {
    m_bCurrentBaseNotFound = false;
    auto &matload = WorldState::get()->getMaterialLoader();

    std::string baseMatName = getBaseMaterialName();
    std::string vpath = matload.getBaseYamlPath(baseMatName);

    try {
        fs::path path = getFileSystem().findExistingFile(vpath, std::nothrow);

        if (path.empty()) {
            if (!silenceNotFound) {
                printe("Failed to load {}: not found", vpath);
            }

            m_bCurrentBaseNotFound = true;
        }

        std::ifstream file(path);
        YAML::Node yaml = YAML::Load(file);
        m_OrigBase.loadFromYaml(yaml);
        snprintf(m_OrigBase.szName, sizeof(m_OrigBase.szName), "%s", baseMatName.c_str());
        m_Base = m_OrigBase;
    } catch (const std::exception &e) {
        printe("Failed to load {}: {}", vpath, e.what());
    }
}

void SurfaceEditMode::SurfaceData::loadSurfaceProps() {
    std::string vpath = getSurfaceYamlPath();
    fs::path path = getFileSystem().findExistingFile(vpath, std::nothrow);

    if (!path.empty()) {
        try {
            std::ifstream file(path);
            m_SurfaceYaml = YAML::Load(file);
            std::string idx = std::to_string(m_iIdx);
            
            if (m_SurfaceYaml[idx]) {
                m_OrigSurf.loadFromYaml(m_SurfaceYaml[idx]);
                m_Surf = m_OrigSurf;
            }
        } catch (const std::exception &e) {
            printe("Failed to load {}: {}", vpath, e.what());
        }
    }
}

template <typename T, bool bAllowRemove>
void SurfaceEditMode::SurfaceData::saveFile(const std::string &vpath, T &data, T &origData) {
    if (data.isIdentical(origData)) {
        return;
    }

    fs::path path = getFileSystem().getFilePath(vpath);

    if constexpr (bAllowRemove) {
        if (data.isIdentical(data.getDefault())) {
            // Delete the file
            if (fs::exists(path)) {
                fs::remove(path);
            }
            return;
        }
    }

    try {
        fs::create_directories(path.parent_path());
        std::ofstream file;
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        file.open(path);

        YAML::Node node;
        data.saveToYaml(node);
        file << node << "\n";
    } catch (const std::exception &e) {
        printe("Failed to save {}: {}", vpath, e.what());
    }
}

void SurfaceEditMode::SurfaceData::saveSurfaceProps() {
    if (m_Surf.isIdentical(m_OrigSurf)) {
        return;
    }

    std::string vpath = getSurfaceYamlPath();
    fs::path path = getFileSystem().getFilePath(vpath);
    std::string idx = std::to_string(m_iIdx);

    if (m_SurfaceYaml[idx]) {
        YAML::Node node = m_SurfaceYaml[idx];
        m_Surf.saveToYaml(node);
    } else {
        YAML::Node newNode;
        m_Surf.saveToYaml(newNode);
        m_SurfaceYaml[idx] = newNode;
    }

    try {
        fs::create_directories(path.parent_path());
        std::ofstream file;
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        file.open(path);
        file << m_SurfaceYaml << "\n";
    } catch (const std::exception &e) {
        printe("Failed to save {}: {}", vpath, e.what());
    }
}

SurfaceEditMode::SurfaceEditMode()
    : m_SelectTool(this) {
    registerTool(&m_SelectTool);
    activateTool(&m_SelectTool);
}

const char *SurfaceEditMode::getName() {
    return "Surface";
}

void SurfaceEditMode::onLevelUnloaded() {
    m_pSurfaceData = nullptr;
}

void SurfaceEditMode::showInspector() {
    int surfIdx = m_SelectTool.getSurfaceIndex();
    int entIdx = m_SelectTool.getEntityIndex();

    if (surfIdx != -1) {
        if (m_pSurfaceData && m_pSurfaceData->getIdx() != surfIdx) {
            m_pSurfaceData = nullptr;
        }

        if (!m_pSurfaceData) {
            m_pSurfaceData = std::make_unique<SurfaceData>(this, surfIdx);
        }

        // Surface info
        ImGui::PushID("surface info");
        if (ImGui::BeginTable("table", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableNextColumn();
            Material *material = MainView::get()->getSurfaceMaterial(surfIdx);
            float width, height;

            if (material->getWide() <= material->getTall()) {
                height = TEXTURE_PREVIEW_SIZE;
                width = height / material->getTall() * material->getWide();
            } else {
                width = TEXTURE_PREVIEW_SIZE;
                height = width / material->getWide() * material->getTall();
            }

            ImGui::Image(material, ImVec2(width, height));

            ImGui::TableNextColumn();
            ImGui::Text("Surface %d", surfIdx);

            if (entIdx == -1) {
                ImGui::Text("World");
            } else {
                const std::string &entClassName =
                    WorldState::get()->getEntList()[entIdx]->getClassName();
                ImGui::Text("Entity %d (%s)", entIdx, entClassName.c_str());
            }

            ImGui::Text("Material %s\n%d x %d", material->getName().c_str(), material->getWide(),
                        material->getTall());

            const bsp::BSPFace &face = WorldState::get()->getLevel().getFaces()[surfIdx];
            ImGui::Text("Lightstyles: %d %d %d %d", face.nStyles[0], face.nStyles[1],
                        face.nStyles[2], face.nStyles[3]);

            ImGui::EndTable();
        }
        ImGui::PopID();

        // Surface
        ImGui::PushID("Surface");
        if (ImGui::CollapsingHeader("Surface", ImGuiTreeNodeFlags_DefaultOpen)) {
            m_pSurfaceData->showSurfaceProps();
        }
        ImGui::PopID();

        // Map Material
        ImGui::PushID("Map Material");
        if (ImGui::CollapsingHeader("Map material", ImGuiTreeNodeFlags_DefaultOpen)) {
            m_pSurfaceData->showMapMatProps();
        }
        ImGui::PopID();

        // Global Material
        ImGui::PushID("Global Material");
        if (ImGui::CollapsingHeader("Global material", ImGuiTreeNodeFlags_DefaultOpen)) {
            m_pSurfaceData->showWadMatProps();
        }
        ImGui::PopID();

        // Base Material
        ImGui::PushID("Base Material");
        if (ImGui::CollapsingHeader("Base material", ImGuiTreeNodeFlags_DefaultOpen)) {
            m_pSurfaceData->showBaseMatProps();
        }
        ImGui::PopID();
    } else {
        if (m_pSurfaceData) {
            m_pSurfaceData = nullptr;
        }

        ImGui::Text("No surface selected.");
    }
}
