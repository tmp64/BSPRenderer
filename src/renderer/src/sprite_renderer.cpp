#include <renderer/sprite_model.h>
#include <renderer/scene_shaders.h>
#include <renderer/utils.h>
#include "sprite_renderer.h"

SceneRenderer::SpriteRenderer::SpriteRenderer(SceneRenderer &renderer)
    : m_Renderer(renderer) {
    createSpriteVbo();
    createSpriteVao();
}

void SceneRenderer::SpriteRenderer::drawSpriteEntity(const ViewContext &context,
                                                     ClientEntity *clent) {
    SpriteModel &model = static_cast<SpriteModel &>(*clent->pModel);
    glm::vec3 spriteScale =
        glm::vec3(1, model.spriteInfo.width, model.spriteInfo.height) * clent->flScale;
    glm::mat4 transform = glm::scale(glm::identity<glm::mat4>(), spriteScale);

    float entRoll =
        clent->vAngles[ANGLE_ROLL] != 0 ? clent->vAngles[ANGLE_ROLL] : clent->vAngles[ANGLE_YAW];
    glm::vec3 angles = clent->vAngles;
    glm::vec3 camAngles = context.getViewAngles();

    switch (model.spriteInfo.type) {
    case bsp::SPR_VP_PARALLEL_UPRIGHT: {
        // Yaw and roll copied from the camera
        angles[ANGLE_YAW] = camAngles[ANGLE_YAW];
        angles[ANGLE_ROLL] = camAngles[ANGLE_ROLL] + entRoll;
        break;
    }
    case bsp::SPR_FACING_UPRIGHT: {
        // Yaw adjusted to point at the camera
        // TODO:
        break;
    }
    case bsp::SPR_VP_PARALLEL: {
        // Pitch, yaw and roll copied from the camera
        angles = camAngles;
        break;
    }
    case bsp::SPR_ORIENTED: {
        // Uses entity PYR
        angles = clent->vAngles;
        break;
    }
    case bsp::SPR_VP_PARALLEL_ORIENTED: {
        // Pitch, yaw and roll copied from the camera
        // Also adds entity roll
        angles = camAngles;
        angles[ANGLE_ROLL] += entRoll;
        break;
    }
    }

    transform = getEntityTransform(clent->vOrigin, angles) * transform;

    // Draw the sprite
    glBindVertexArray(m_SpriteVao);
    m_Renderer.setRenderMode(clent->iRenderMode);
    Material &mat = *model.spriteMat;
    mat.activateTextures();
    auto &shader = mat.enableShader(SHADER_TYPE_CUSTOM_IDX, m_Renderer.m_uFrameCount)
                       ->getShader<SceneShaders::SpriteShader>();
    shader.setModelMatrix(transform);
    shader.setRenderMode(clent->iRenderMode);
    shader.setRenderFx(clent->iFxAmount / 255.0f, glm::vec3(clent->vFxColor) / 255.0f);
    shader.setFrame(clent->flFrame);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void SceneRenderer::SpriteRenderer::createSpriteVbo() {
    // clang-format off
    
    //! Sprite vertex buffer. A quad positioned to look in -X direction.
    //! Unrotated camera's forward direction is +X.
    //! This quad will be rotated like camera so it's always parallel to camera's clipping planes.
    SpriteVertex vertices[] = {
        {{0.0f, 0.5f, -0.5f},  {0.0f, 1.0f}},
        {{0.0f, 0.5f, 0.5f},   {0.0f, 0.0f}},
        {{0.0f, -0.5f, 0.5f},  {1.0f, 0.0f}},
        {{0.0f, -0.5f, -0.5f}, {1.0f, 1.0f}},
    };
    // clang-format on

    m_SpriteVbo.create(GL_ARRAY_BUFFER, "Sprite VBO");
    m_SpriteVbo.init(sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void SceneRenderer::SpriteRenderer::createSpriteVao() {
    m_SpriteVao.create();
    glBindVertexArray(m_SpriteVao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    m_SpriteVbo.bind();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          (void *)offsetof(SpriteVertex, position));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          (void *)offsetof(SpriteVertex, texture));
    glBindVertexArray(0);
}

glm::mat4 SceneRenderer::SpriteRenderer::getEntityTransform(glm::vec3 origin, glm::vec3 angles) {
    glm::mat4 modelMat = glm::identity<glm::mat4>();
    modelMat = glm::translate(modelMat, origin);
    modelMat = glm::rotate(modelMat, glm::radians(angles.y), {0.0f, 0.0f, 1.0f});
    modelMat = glm::rotate(modelMat, glm::radians(angles.x), {0.0f, 1.0f, 0.0f});
    modelMat = glm::rotate(modelMat, glm::radians(angles.z), {1.0f, 0.0f, 0.0f});
    return modelMat;
}
