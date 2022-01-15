#ifndef MATERIAL_SYSTEM_MATERIAL_SYSTEM_H
#define MATERIAL_SYSTEM_MATERIAL_SYSTEM_H
#include <app_base/app_component.h>
#include <graphics/texture.h>
#include <material_system/shader_definitions.h>
#include <material_system/material.h>

class Shader;

class MaterialSystem : public AppComponentBase<MaterialSystem> {
public:
    class GraphicsSettings {
    public:
        //! Maximum ever possible anisotropy level.
        static constexpr int MAX_ANISOTROPY = 16;

        inline TextureFilter getFilter() const { return m_Filter; }
        inline int getAniso() const { return m_iAniso; }

        inline void setFilter(TextureFilter filter) {
            m_Filter = filter;
            m_bDirty = true;
        }

        inline void setAniso(int aniso) {
            m_iAniso = std::clamp(aniso, 1, MAX_ANISOTROPY);
            m_bDirty = true;
        }

        //! @returns whether any settings were changed since last time they were applied.
        inline bool isDirty() const { return m_bDirty; }

        //! Resets the dirty flag.
        inline void resetDirty() { m_bDirty = false; }

    private:
        bool m_bDirty = true;
        TextureFilter m_Filter = TextureFilter::Nearest;
        int m_iAniso = 1;
    };

    MaterialSystem();
    ~MaterialSystem();

    void tick() override;
    void lateTick() override;

    //! Returns the default black-purple material.
    Material *getNullMaterial();

    //! Creates a new material.
    Material *createMaterial(std::string_view name);

    //! Destroys a material. mat will become an invalid pointer.
    void destroyMaterial(Material *mat);

    //! Reloads all shaders.
    bool reloadShaders();

    //! Applies graphics settings to a texture,
    void applyGraphicsSettings(Texture &texture);

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

    GraphicsSettings m_Settings;

    void unloadShaders();
    void createNullMaterial();
};

#endif
