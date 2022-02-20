#ifndef LIGHTMAP_IFACE_H
#define LIGHTMAP_IFACE_H
#include <renderer/scene_renderer.h>

class SceneRenderer::ILightmap {
public:
    virtual ~ILightmap() = default;

    //! @returns gamma of the lightmap
    virtual float getGamma() = 0;

    //! Binds the array texture of the lightmap.
    virtual void bindTexture() = 0;

    //! Binds the VBO.
    virtual void bindVertBuffer() = 0;

    //! Updates texture filtering properties.
    virtual void updateFilter(bool filterEnabled) = 0;
};

#endif
