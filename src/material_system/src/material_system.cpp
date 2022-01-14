#include <graphics/texture2d.h>
#include <material_system/material_system.h>
#include <material_system/shader.h>
#include <material_system/shader_instance.h>

ConVar<bool> mat_ui("mat_ui", false, "");

namespace {

class FallbackShader : public ShaderT<FallbackShader> {
public:
    FallbackShader(unsigned typeIdx = 0)
        : BaseClass(typeIdx) {
        setTitle("MaterialSystem::FallbackShader");
        setVert("assets:shaders/fallback.vert");
        setFrag("assets:shaders/fallback.frag");
        setTypes(SHADER_TYPE_ALL);
    }
};

FallbackShader s_FallbackShader;

}

MaterialSystem::MaterialSystem() {
    setTickEnabled(true);

    addVertexShaderDef("IFACE_VF", "out");
    addFragmentShaderDef("IFACE_VF", "in");

    reloadShaders();
    createNullMaterial();
}

MaterialSystem::~MaterialSystem() {
    unloadShaders();
}

void MaterialSystem::tick() {}

Shader *MaterialSystem::getFallbackShader() {
    return &s_FallbackShader;
}

Material *MaterialSystem::getNullMaterial() {
    return &(*m_Materials.begin());
}

Material *MaterialSystem::createMaterial(std::string_view name) {
    auto it = m_Materials.emplace(m_Materials.end(), name);
    it->m_Iter = it;
    return &(*it);
}

void MaterialSystem::destroyMaterial(Material *mat) {
    m_Materials.erase(mat->m_Iter);
}

void MaterialSystem::reloadShaders() {
    unloadShaders();

    auto &prototypes = Shader::getPrototypeList();

    for (Shader *pShader : prototypes) {
        pShader->createShaderInstances();
    }
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
    Material *mat = createMaterial("Null Material");
    mat->setSize(size, size);
    mat->setTexture(0, std::make_unique<Texture2D>());
    
    auto tex = static_cast<Texture2D *>(mat->getTexture(0));
    tex->create("Null Texture");
    tex->initTexture(GraphicsFormat::RGB8, size, size, false, GL_RGB, GL_UNSIGNED_BYTE,
                     CheckerboardImage::get().data.data());
}
