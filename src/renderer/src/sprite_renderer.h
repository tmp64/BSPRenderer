#ifndef SPRITE_RENDERER_H
#define SPRITE_RENDERER_H
#include <renderer/scene_renderer.h>

class SceneRenderer::SpriteRenderer {
public:
    SpriteRenderer(SceneRenderer &renderer);

    //! Draws the sprite
    void drawSpriteEntity(const ViewContext &context, ClientEntity *clent);

private:
    struct SpriteVertex {
        glm::vec3 position;
        glm::vec2 texture;
    };

    SceneRenderer &m_Renderer;
    GPUBuffer m_SpriteVbo;
    GLVao m_SpriteVao;

    void createSpriteVbo();
    void createSpriteVao();

    static glm::mat4 getEntityTransform(glm::vec3 origin, glm::vec3 angles);
};

#endif
