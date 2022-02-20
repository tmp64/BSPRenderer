#ifndef FAKE_LIGHTMAP_H
#define FAKE_LIGHTMAP_H
#include <graphics/texture2d_array.h>
#include <renderer/scene_renderer.h>
#include "lightmap_iface.h"

class SceneRenderer::FakeLightmap : public SceneRenderer::ILightmap {
public:
    FakeLightmap(SceneRenderer &renderer);
    float getGamma() override;
    void bindTexture() override;
    void bindVertBuffer() override;
    void updateFilter(bool filterEnabled) override;

private:
    Texture2DArray m_Texture;
    GPUBuffer m_VertBuffer;
};

#endif
