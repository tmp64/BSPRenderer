#include <graphics/texture2d.h>
#include <material_system/material_system.h>
#include <material_system/shader.h>
#include <material_system/shader_instance.h>
#include <imgui.h>

static constexpr int DEFAULT_FILTER = 2;
static constexpr int DEFAULT_ANISO = 16;

ConVar<bool> mat_ui("mat_ui", false, "");

ConVar<int> mat_filter("mat_filter", 2, "Default filtering mode: nearest, bilinear, trilinear");
ConVar<int> mat_aniso("mat_aniso", 16, "Default anisotropy level (1-16)");

ConCommand
    cmd_mat_reloadshaders("mat_reloadshaders",
                          "Reloads all shaders. If any fail to compile, they won't be reloaded.");

void MaterialDeleter::operator()(Material *mat) {
    if (mat) {
        MaterialSystem::get().destroyMaterial(mat);
    }
}

MaterialSystem::MaterialSystem() {
    setTickEnabled(true);

    addVertexShaderDef("IFACE_VF", "out");
    addFragmentShaderDef("IFACE_VF", "in");

    if (!reloadShaders()) {
        throw std::runtime_error("Some shaders failed to compile. Check the log ofr details.");
    }

    createNullMaterial();

    cmd_mat_reloadshaders.setCallback([&]() { reloadShaders(); });

    mat_filter.setCallback([&](const int &, const int &newVal) {
        TextureFilter filter = (TextureFilter)std::clamp(newVal, 0, 2);
        m_Settings.setFilter(filter);
        return true;
    });

    mat_aniso.setCallback([&](const int &, const int &newVal) {
        m_Settings.setAniso(newVal);
        return true;
    });

    mat_filter.setValue(DEFAULT_FILTER);
    mat_aniso.setValue(DEFAULT_ANISO);
}

MaterialSystem::~MaterialSystem() {
    unloadShaders();
}

void MaterialSystem::tick() {
    if (mat_ui.getValue()) {
        bool isOpen = true;
        ImGui::SetNextWindowBgAlpha(0.2f);
        if (ImGui::Begin("Material System", &isOpen)) {
            ImGui::Text("Count: %d", (int)m_Materials.size());

            const char *filters[] = {"Nearest", "Bilinear", "Trilinear"};
            int curFilter = (int)m_Settings.getFilter();

            if (ImGui::BeginCombo("Filtering", filters[curFilter])) {
                for (int i = 0; i < 3; i++) {
                    bool isSelected = i == curFilter;

                    if (ImGui::Selectable(filters[i], isSelected)) {
                        mat_filter.setValue(i);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            int aniso = m_Settings.getAniso();
            if (ImGui::SliderInt("Anisotropic filtering", &aniso, 1, GraphicsSettings::MAX_ANISOTROPY)) {
                mat_aniso.setValue(aniso);
            }
        }

        ImGui::End();

        if (!isOpen) {
            mat_ui.setValue(isOpen);
        }
    }
}

void MaterialSystem::lateTick() {
    if (m_Settings.isDirty()) {
        m_Settings.resetDirty();

        for (Material &mat : m_Materials) {
            if (mat.getUsesGraphicalSettings()) {
                for (int i = 0; i < Material::MAX_TEXTURES; i++) {
                    Texture *texture = mat.getTexture(i);
                    if (texture) {
                        applyGraphicsSettings(*texture);
                    }
                }
            }
        }
    }
}

Material *MaterialSystem::getNullMaterial() {
    return &(*m_Materials.begin());
}

MaterialPtr MaterialSystem::createMaterial(std::string_view name) {
    auto it = m_Materials.emplace(m_Materials.end(), name);
    it->m_Iter = it;
    return MaterialPtr(&(*it));
}

void MaterialSystem::destroyMaterial(Material *mat) {
    m_Materials.erase(mat->m_Iter);
}

bool MaterialSystem::reloadShaders() {
    auto &prototypes = Shader::getPrototypeList();
    bool success = true;

    for (Shader *pShader : prototypes) {
        if (!pShader->createShaderInstances()) {
            success = false;
        }
    }

    return success;
}

void MaterialSystem::applyGraphicsSettings(Texture &texture) {
    texture.setFilter(m_Settings.getFilter());
    texture.setAnisoLevel(m_Settings.getAniso());
}

void MaterialSystem::unloadShaders() {
    // Make sure no dangling pointers are stored
    ShaderInstance::disable();

    auto &prototypes = Shader::getPrototypeList();

    for (Shader *pShader : prototypes) {
        pShader->freeShaderInstances();
    }
}

void MaterialSystem::createNullMaterial() {
    int size = CheckerboardImage::get().size;
    m_NullMaterial = createMaterial("Null Material");
    m_NullMaterial->setSize(size, size);
    m_NullMaterial->setTexture(0, std::make_unique<Texture2D>());
    
    auto tex = static_cast<Texture2D *>(m_NullMaterial->getTexture(0));
    tex->create("Null Texture");
    tex->initTexture(GraphicsFormat::RGB8, size, size, false, GL_RGB, GL_UNSIGNED_BYTE,
                     CheckerboardImage::get().data.data());
}
