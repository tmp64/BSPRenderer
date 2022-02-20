#ifndef RENDERER_RENDERER_ENGINE_INTERFACE_H
#define RENDERER_RENDERER_ENGINE_INTERFACE_H
#include <bsp/bsp_types.h>

class Material;

class IRendererEngine {
public:
    virtual ~IRendererEngine() = default;

    //! Returns the material for specified texture or nullptr.
    virtual Material *getMaterial(const bsp::BSPMipTex &tex) = 0;

    //! Draws opaque triangles.
    virtual void drawNormalTriangles(unsigned &drawcalls) = 0;

    //! Draws transparent triangles.
    virtual void drawTransTriangles(unsigned &drawcalls) = 0;
};

#endif
