#ifndef CUSTOM_LIGHTMAP_H
#define CUSTOM_LIGHTMAP_H
#include <appfw/binary_file.h>
#include <graphics/texture2d_array.h>
#include <renderer/scene_renderer.h>
#include "lightmap_iface.h"

class SceneRenderer::CustomLightmap : public SceneRenderer::ILightmap {
public:
    CustomLightmap(SceneRenderer &renderer);
    float getGamma() override;
    void bindTexture() override;
    void bindVertBuffer() override;
    void updateFilter(bool filterEnabled) override;

private:
    Texture2DArray m_Texture;
    GPUBuffer m_VertBuffer;
    GPUBuffer m_PatchVertBuffer;

    void readTexture(appfw::BinaryInputFile &file, glm::ivec2 size);
};

#endif
