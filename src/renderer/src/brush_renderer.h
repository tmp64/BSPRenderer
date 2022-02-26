#ifndef BRUSH_RENDERER_H
#define BRUSH_RENDERER_H
#include <renderer/scene_renderer.h>

class SceneRenderer::BrushRenderer {
public:
    BrushRenderer(SceneRenderer &renderer);

    //! Called when bmodels are rendered again for a different context.
    //! Resets the EBO.
    void beginRendering();

    //! Renders a brush model without sorting its surfaces.
    //! Good for opaque entities.
    void drawBrushEntity(const ViewContext &context, ClientEntity *clent);

    //! Renders a brush model with surfaces sorted.
    //! Provides correct blending of transparent surfaces.
    void drawSortedBrushEntity(const ViewContext &context, ClientEntity *clent);

private:
    struct EntData {
        glm::mat4 transformMat;
    };

    SceneRenderer &m_Renderer;
    std::vector<unsigned> m_SortBuffer;

    // Batching
    GPUBuffer m_WorldEbo;
    std::vector<uint16_t> m_WorldEboBuf;
    unsigned m_uWorldEboOffset = 0;
    unsigned m_uWorldEboIdx = 0;
    Material *m_pBatchMaterial = nullptr;

    void createBuffers();

    //! Prepares brush model entity for rendering.
    //! @returns whether the entity was culled.
    bool prepareEntity(const ViewContext &context, const ClientEntity *clent, EntData &entData) const;

    //! @returns brush model transformation materix.
    glm::mat4 getBrushTransform(const ClientEntity *clent) const;

    //! Adds a surface to the current batch.
    void batchAddSurface(const Surface &surf);

    //! Issues current batch for rendering.
    void batchFlush();

    //! Resets current batch. Pending data is discarded.
    void batchReset();
};

#endif
