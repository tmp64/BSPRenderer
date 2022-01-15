#ifndef MATERIAL_SYSTEM_MATERIAL_H
#define MATERIAL_SYSTEM_MATERIAL_H
#include <appfw/utils.h>
#include <graphics/texture.h>
#include <material_system/shader.h>
#include <material_system/shader_instance.h>

class MaterialSystem;

//! A checkerboard pattern of black and purple pixels.
struct CheckerboardImage : appfw::NoCopy {
    std::vector<uint8_t> data;
    int size;

    CheckerboardImage();

    static const CheckerboardImage &get();
};

class Material {
public:
    //! Number of textures that a material can own.
    static constexpr int MAX_TEXTURES = 1;

    //! Do not use directly, use MaterialSystem instead.
    Material(std::string_view name);

    //! Binds material's textures into TUs [0, MAX_TEXTURES).
    void activateTextures() const;

    //! Enables the shader.
    ShaderInstance *enableShader(unsigned typeIdx, unsigned curFrame) const;

    inline int getWide() const { return m_iWide; }
    inline int getTall() const { return m_iTall; }
    inline const std::string &getName() const { return m_Name; }
    inline const std::string &getWadName() const { return m_WadName; }
    inline Texture *getTexture(int idx) const { return m_pOwnTextures[idx].get(); }
    inline bool getUsesGraphicalSettings() const { return m_bUseGraphicalSettings; }
    inline ShaderInstance *getShader(unsigned typeIdx) const {
        return m_pShaders[typeIdx]->getShaderInstance(typeIdx);
    }

    //! Sets the size of the material in hammer units.
    //! It has nothing to do with actual texture size.
    //! Can be set to 1x1 if isn't used in scene renderer.
    void setSize(int wide, int tall);

    //! Sets the name of the WAD (if any).
    void setWadName(std::string_view wadName);

    //! Sets the texture at index. The texture will be owned by the material.
    void setTexture(int idx, std::unique_ptr<Texture> &&pTexture);

    //! Sets whether the material's textures should use global graphical settings.
    void setUsesGraphicalSettings(bool value);

    //! Sets the material's shader.
    void setShader(unsigned typeIdx, Shader *shader);

private:
    int m_iWide = 0;
    int m_iTall = 0;
    std::list<Material>::iterator m_Iter;
    std::string m_Name;
    std::string m_WadName;
    std::unique_ptr<Texture> m_pOwnTextures[MAX_TEXTURES];
    bool m_bUseGraphicalSettings = false;
    Shader *m_pShaders[MAX_SHADER_TYPE_COUNT] = {nullptr};

    friend class MaterialSystem;
};

#endif
