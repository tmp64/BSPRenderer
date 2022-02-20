#ifndef BSP_LIGHTMAP_H
#define BSP_LIGHTMAP_H
#include <graphics/texture2d_array.h>
#include <renderer/scene_renderer.h>
#include "lightmap_iface.h"

class SceneRenderer::BSPLightmap : public SceneRenderer::ILightmap {
public:
    BSPLightmap(SceneRenderer &renderer);
    float getGamma() override;
    void bindTexture() override;
    void bindVertBuffer() override;
    void updateFilter(bool filterEnabled) override;

private:
    static constexpr int LIGHTMAP_DIVISOR = 16;
    static constexpr int MAX_LIGHTMAP_BLOCK_SIZE = 2048;
    static constexpr float LIGHTMAP_BLOCK_WASTED = 0.40f; //!< How much area is assumed to be wasted due to packing
    static constexpr int LIGHTMAP_PADDING = 2;
    static constexpr float LIGHTMAP_GAMMA = 2.5f;

    struct SurfaceData {
        glm::vec2 textureMins = glm::vec2(0, 0);
        glm::ivec2 size = glm::ivec2(0, 0);
        glm::ivec2 offset = glm::ivec2(0, 0);
    };

    Texture2DArray m_Texture;
    GPUBuffer m_VertBuffer;
};

#endif
