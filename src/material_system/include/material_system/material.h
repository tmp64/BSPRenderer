#ifndef MATERIAL_SYSTEM_MATERIAL_H
#define MATERIAL_SYSTEM_MATERIAL_H
#include <appfw/utils.h>
#include <graphics/texture.h>

class MaterialSystem;
class Shader;

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

    //! Binds material's textures into TUs [0, MAX_TEXTURES).
    void activateTextures() const;

    //! Enables the shader.
    void enableShader(unsigned typeIdx) const;

    inline int getWide() const { return m_iWide; }
    inline int getTall() const { return m_iWide; }
    inline std::string_view getName() const { return m_Name; }
    inline std::string_view getWadName() const { return m_WadName; }
    inline Texture *getTexture(int idx) const { return m_pOwnTextures[idx].get(); }
    inline bool getUsesGraphicalSettings() const { return m_bUseGraphicalSettings; }

    //! Sets the size of the material in hammer units.
    void setSize(int wide, int tall);

    //! Sets the name of the WAD (if any).
    void setWadName(std::string_view wadName);

    //! Sets the texture at index. The texture will be owned by the material.
    void setTexture(int idx, std::unique_ptr<Texture> &&pTexture);

    //! Sets whether the material's textures should use global graphical settings.
    void setUsesGraphicalSettings(bool value);

private:
    int m_iWide = 0;
    int m_iTall = 0;
    std::list<Material>::iterator m_Iter;
    std::string m_Name;
    std::string m_WadName;
    std::unique_ptr<Texture> m_pOwnTextures[MAX_TEXTURES];
    bool m_bUseGraphicalSettings = false;
    Shader *m_pShader = nullptr;

    Material(std::string_view name);

    friend class MaterialSystem;
};

#endif
