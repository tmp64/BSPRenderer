#ifndef RENDERER_MATERIAL_MANAGER_H
#define RENDERER_MATERIAL_MANAGER_H
#include <appfw/utils.h>
#include <cstddef>
#include <glad/glad.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <renderer/gpu_managed_objects.h>
#include <app_base/app_component.h>

struct CheckerboardImage : appfw::NoCopy {
    std::vector<uint8_t> data;
    int size;

    CheckerboardImage();

    static const CheckerboardImage &get();
};

class Material : appfw::NoCopy {
public:
    enum class Type
    {
        None,
        Surface, //!< A 2D material
    };

    Material(std::string_view name);
    Material(std::nullptr_t);

    //! Returns name of the material.
    inline const std::string &getName() const { return m_Name; }

    //! Binds the color texture to GL_TEXTURE0.
    inline void bindSurfaceTextures() const {
        AFW_ASSERT(m_Type == Type::Surface);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_Texture.getId());
    }

    //! Type of the material
    inline Type getType() const { return m_Type; }

    //! Width of the material.
    inline int getWide() const { return m_iWide; }

    //! Height of the material.
    inline int getTall() const { return m_iTall; }

    //! Sets the materials's RGB8 color data
    void setImageRGB8(int wide, int tall, const void *data);

    //! Sets the materials's RGBA8 color data
    void setImageRGBA8(int wide, int tall, const void *data);

private:
    std::list<Material>::iterator m_Iter; 
    std::string m_Name;
    GPUTexture m_Texture;
    Type m_Type = Type::None;
    int m_iWide = 0;
    int m_iTall = 0;

    void initSurface();

    friend class MaterialManager;
};

/**
 * Provides access to materials.
 */
class MaterialManager : public AppComponentBase<MaterialManager> {
public:
    MaterialManager();
    ~MaterialManager();

    //! Per-tick update.
    void tick() override;

    //! Returns the default black-purple material.
    Material *getNullMaterial();

    //! Creates a new material.
    Material *createMaterial(std::string_view name);

    //! Destroys a material. mat will become an invalid pointer.
    void destroyMaterial(Material *mat);

    //! Checks if anisotropic filtering is supported.
    bool isAnisoSupported();

private:
    // Filtering settings
    float m_flMaxAniso = 0;
    bool m_bLinearFiltering = false;
    int m_iMipMapping = 0;
    int m_iAniso = 1;

    // Matrials
    std::list<Material> m_Materials;

    //! Updates texture filtering settings of currently bound texture.
    void updateTextureFiltering(GLenum target);

    friend class Material;
};

#endif
