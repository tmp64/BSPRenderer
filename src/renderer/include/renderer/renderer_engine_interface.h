#ifndef RENDERER_RENDERER_ENGINE_INTERFACE_H
#define RENDERER_RENDERER_ENGINE_INTERFACE_H

class Material;

class IRendererEngine {
public:
    virtual ~IRendererEngine() = default;

    //! Returns the material for specified texture or nullptr.
    virtual Material *getMaterial(const bsp::BSPMipTex &tex) = 0;

    //! Draws opaque triangles.
    virtual void drawNormalTriangles(unsigned &drawcallCount) = 0;

    //! Draws transparent triangles.
    virtual void drawTransTriangles(unsigned &drawcallCount) = 0;
};

#endif
