#ifndef RAD_MATERIAL_H
#define RAD_MATERIAL_H
#include <bsp/wad_file.h>
#include <bsp/level.h>
#include <material_props/material_props.h>
#include <material_props/material_prop_loader.h>

namespace rad {

class RadSimImpl;

class Material {
public:
    //! Loads properties for the specified material.
    //! @param  loader  material prop loader
    //! @param  texName Name of the material
    //! @param  wadName Name of the WAD
    void loadProps(MaterialPropLoader &loader, std::string_view texName, std::string_view wadName);

    //! Loads a texture from packed RGB data.
    void loadFromRGB(RadSimImpl &radSim, int wide, int tall, const uint8_t *data);

    //! Loads a texture from packed RGBA data. Alpha is ignored.
    void loadFromRGBA(RadSimImpl &radSim, int wide, int tall, const uint8_t *data);

    //! Loads color texture from a WAD.
    void loadFromWad(RadSimImpl &radSim, const bsp::WADTexture &wadTex);

    //! Loads color texture from the level.
    //! @param  level       The level.
    //! @param  texIndex    Index of miptex.
    void loadFromLevel(const bsp::Level &level, int texIndex);

    //! @returns the material properties.
    inline const MaterialProps &getProps() { return m_Props; }

    //! @returns whether color can be sampled
    inline bool hasColorTexture() { return !m_Pixels.empty(); }

    //! Samples color around a point.
    //! @param  pos     Sample position
    //! @param  scale   Filter scale in pixels. This should be pixel width of the largest pixel.
    glm::vec3 sampleColor(glm::vec2 pos, glm::vec2 scale);

    //! Reads a color of a pixel. Coordinates wrap around.
    inline glm::vec3 readColor(int x, int y) {
        static_assert((11 % 5) == 1);
        static_assert(((-1 % 5 + 5) % 5) == 4);
        x = (x % m_iWide + m_iWide) % m_iWide;
        y = (y % m_iTall + m_iTall) % m_iTall;
        return m_Pixels[m_iWide * y + x];
    }

private:
    MaterialProps m_Props;
    int m_iWide = 0;
    int m_iTall = 0;
    std::vector<glm::vec3> m_Pixels;
};

} // namespace rad

#endif
