#ifndef RENDERER_MATERIAL_MANAGER_H
#define RENDERER_MATERIAL_MANAGER_H
#include <appfw/utils.h>
#include <bsp/wad_texture.h>
#include <cstddef>
#include <glad/glad.h>
#include <vector>
#include <unordered_map>

constexpr size_t NULL_MATERIAL = 0;

struct CheckerboardImage : appfw::NoCopy {
    std::vector<uint8_t> data;
    int size;

    CheckerboardImage();

    static const CheckerboardImage &get();
};

/**
 * A material that can have multiple textures.
 */
class Material : appfw::NoCopy {
public:
    Material() = default;
    Material(std::nullptr_t);
    Material(const bsp::WADTexture &texture);
    Material(Material &&from) noexcept;
    ~Material();

    /**
     * Returns name of the material.
     */
    inline const std::string &getName() const { return m_Name; }

    /**
     * Binds texture to GL_TEXTURE0.
     */
    inline void bindTextures() const {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_nTexture);
    }

    /**
     * Width of the material texture.
     */
    inline int getWide() const { return m_iWide; }

    /**
     * Height og the material texture.
     */
    inline int getTall() const { return m_iTall; }

private:
    std::string m_Name;
    GLuint m_nTexture = 0;
    int m_iWide = 0;
    int m_iTall = 0;
};

/**
 * Loads WAD files and provides access to materials.
 */
class MaterialManager {
public:
    static MaterialManager &get();
    
    /**
     * Initializes the material manager.
     */
    void init();

    /**
     * Releases used resources.
     */
    void shutdown();

    /**
     * Loads a WAD file into the manager.
     */
    void addWadFile(const fs::path &name);

    /**
     * Returns an index of specified material or NULL_MATERIAL if not found.
     */
    size_t findMaterial(const std::string &name);

    /**
     * Returns a material.
     * Don't keep the reference as it may be invalidated.
     * @param   index   An index returned by findMaterial. Must be valid or behavior is undefined.
     */
    inline const Material &getMaterial(size_t index) { return m_Materials[index]; }

private:
    std::vector<Material> m_Materials;
    std::unordered_map<std::string, size_t> m_Map;
};

#endif
