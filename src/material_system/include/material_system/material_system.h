#ifndef MATERIAL_SYSTEM_MATERIAL_SYSTEM_H
#define MATERIAL_SYSTEM_MATERIAL_SYSTEM_H
#include <app_base/app_component.h>
#include <material_system/shader_definitions.h>

class Shader;

class MaterialSystem : public AppComponentBase<MaterialSystem> {
public:
    MaterialSystem();
    ~MaterialSystem();

    void tick() override;

    //! Reloads all shaders.
    void reloadShaders();

    //! Adds a definition to all shaders.
    template <typename T>
    void addSharedShaderDef(const T &value) {
        m_GlobalShaderDefs.addSharedDef(value);
    }

    //! Adds a definition to all vertex shaders.
    template <typename T>
    void addVertexShaderDef(const T &value) {
        m_GlobalShaderDefs.addVertexDef(value);
    }

    //! Adds a definition to all fragment shaders.
    template <typename T>
    void addFragmentShaderDef(const T &value) {
        m_GlobalShaderDefs.addFragmentDef(value);
    }

    inline std::string getGlobalVertexShaderDefs() { return m_GlobalShaderDefs.toStringVertex(); }
    inline std::string getGlobalFragmentShaderDefs() { return m_GlobalShaderDefs.toStringFragment(); }

private:
    ShaderProgramDefinitions m_GlobalShaderDefs;

    //! Frees all shaders.
    void unloadShaders();
};

#endif
