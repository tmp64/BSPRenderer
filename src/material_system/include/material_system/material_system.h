#ifndef MATERIAL_SYSTEM_MATERIAL_SYSTEM_H
#define MATERIAL_SYSTEM_MATERIAL_SYSTEM_H
#include <app_base/app_component.h>
#include <material_system/shader_definitions.h>
#include <material_system/material.h>

class Shader;

class MaterialSystem : public AppComponentBase<MaterialSystem> {
public:
    MaterialSystem();
    ~MaterialSystem();

    void tick() override;

    //! Returns the default black-purple material.
    Material *getNullMaterial();

    //! Creates a new material.
    Material *createMaterial(std::string_view name);

    //! Destroys a material. mat will become an invalid pointer.
    void destroyMaterial(Material *mat);

    //! Reloads all shaders.
    bool reloadShaders();

    //! Adds a definition to all shaders.
    template <typename T>
    void addSharedShaderDef(std::string_view key, const T &value) {
        m_GlobalShaderDefs.addSharedDef(key, value);
    }

    //! Adds a definition to all vertex shaders.
    template <typename T>
    void addVertexShaderDef(std::string_view key, const T &value) {
        m_GlobalShaderDefs.addVertexDef(key, value);
    }

    //! Adds a definition to all fragment shaders.
    template <typename T>
    void addFragmentShaderDef(std::string_view key, const T &value) {
        m_GlobalShaderDefs.addFragmentDef(key, value);
    }

    inline std::string getGlobalVertexShaderDefs() { return m_GlobalShaderDefs.toStringVertex(); }
    inline std::string getGlobalFragmentShaderDefs() { return m_GlobalShaderDefs.toStringFragment(); }

private:
    // Shaders
    ShaderProgramDefinitions m_GlobalShaderDefs;

    // Materials
    std::list<Material> m_Materials;

    void unloadShaders();
    void createNullMaterial();
};

#endif
