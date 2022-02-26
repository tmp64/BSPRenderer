#include <renderer/utils.h>
#include <renderer/scene_shaders.h>
#include "brush_renderer.h"

SceneRenderer::BrushRenderer::BrushRenderer(SceneRenderer &renderer)
    : m_Renderer(renderer) {
    createBuffers();
}

void SceneRenderer::BrushRenderer::beginRendering() {
    batchReset();
}

void SceneRenderer::BrushRenderer::drawBrushEntity(const ViewContext &context,
                                                        ClientEntity *clent) {
    EntData entData;
    if (prepareEntity(context, clent, entData)) {
        return;
    }

    // Set up GL
    glBindVertexArray(m_Renderer.m_SurfaceVao);
    m_WorldEbo.bind();
    m_Renderer.setRenderMode(clent->iRenderMode);

    // Draw surfaces
    Model *model = clent->pModel;

    for (unsigned i = 0; i < model->uFaceNum; i++) {
        unsigned surfIdx = model->uFirstFace + i;
        const Surface &surf = m_Renderer.m_Surfaces[surfIdx];

        if (surf.material != m_pBatchMaterial) {
            batchFlush();
            m_pBatchMaterial = surf.material;
            m_pBatchMaterial->activateTextures();
            auto &shader = m_pBatchMaterial
                               ->enableShader(SHADER_TYPE_BRUSH_MODEL_IDX, m_Renderer.m_uFrameCount)
                               ->getShader<SceneShaders::BrushShader>();
            shader.setModelMatrix(entData.transformMat);
            shader.setRenderMode(clent->iRenderMode);
            shader.setRenderFx(clent->iFxAmount / 255.f, {0, 0, 0});
        }

        batchAddSurface(surf);
    }

    batchFlush();
    m_Renderer.m_Stats.uBrushEntPolys += model->uFaceNum;
}

void SceneRenderer::BrushRenderer::drawSortedBrushEntity(const ViewContext &context,
                                                         ClientEntity *clent) {
    EntData entData;
    if (prepareEntity(context, clent, entData)) {
        return;
    }

    // Set up GL
    glBindVertexArray(m_Renderer.m_SurfaceVao);
    m_WorldEbo.bind();
    m_Renderer.setRenderMode(clent->iRenderMode);

    Model *model = clent->pModel;
    m_SortBuffer.clear();

    // Sort surfaces
    for (unsigned i = 0; i < model->uFaceNum; i++) {
        unsigned surfIdx = model->uFirstFace + i;
        m_SortBuffer.push_back(surfIdx);
    }

    auto &surfaces = m_Renderer.m_Surfaces;
    auto sortFn = [&](const size_t &lhsIdx, const size_t &rhsIdx) {
        // TODO: Doesn't account for rotation
        glm::vec3 lhsOrg = surfaces[lhsIdx].vOrigin + clent->vOrigin;
        glm::vec3 rhsOrg = surfaces[rhsIdx].vOrigin + clent->vOrigin;
        float lhsDist = glm::dot(lhsOrg, context.getViewForward());
        float rhsDist = glm::dot(rhsOrg, context.getViewForward());

        return lhsDist > rhsDist;
    };

    std::sort(m_SortBuffer.begin(), m_SortBuffer.end(), sortFn);

    // Draw surfaces
    for (unsigned surfIdx : m_SortBuffer) {
        const Surface &surf = m_Renderer.m_Surfaces[surfIdx];

        if (surf.material != m_pBatchMaterial) {
            batchFlush();
            m_pBatchMaterial = surf.material;
            m_pBatchMaterial->activateTextures();
            auto &shader = m_pBatchMaterial
                               ->enableShader(SHADER_TYPE_BRUSH_MODEL_IDX, m_Renderer.m_uFrameCount)
                               ->getShader<SceneShaders::BrushShader>();
            shader.setModelMatrix(entData.transformMat);
            shader.setRenderMode(clent->iRenderMode);
            shader.setRenderFx(clent->iFxAmount / 255.f, glm::vec3(clent->vFxColor) / 255.0f);
        }

        batchAddSurface(surf);
    }

    batchFlush();
    m_Renderer.m_Stats.uBrushEntPolys += model->uFaceNum;
}

void SceneRenderer::BrushRenderer::createBuffers() {
    auto &models = m_Renderer.m_Level.getModels();
    if (models.size() < 2) {
        return;
    }

    // All bmodel faces go after world faces
    unsigned totalSurfCount = (unsigned)m_Renderer.m_Surfaces.size();
    unsigned vertexCount = 0;
    unsigned surfaceCount = 0;
    for (unsigned i = models[1].iFirstFace; i < totalSurfCount; i++) {
        const Surface &surf = m_Renderer.m_Surfaces[i];
        vertexCount += surf.vertexCount;
        surfaceCount++;
    }

    // Maximum ebo size is X vertices + Y primitive restarts.
    // X = max vertex count, Y = max surface count.
    unsigned maxEboSize = vertexCount + surfaceCount;
    m_WorldEboBuf.resize(maxEboSize);
    m_WorldEbo.create(GL_ELEMENT_ARRAY_BUFFER, "BrushRenderer: EBO");
    m_WorldEbo.init(maxEboSize * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);
    printi("BrushRenderer: EBO is {:.1f} KiB", m_WorldEbo.getMemUsage() / 1024.0);
}

bool SceneRenderer::BrushRenderer::prepareEntity(const ViewContext &context,
                                                 const ClientEntity *clent,
                                                 EntData &entData) const {
    Model *model = clent->pModel;

    // Frustum culling
    glm::vec3 mins, maxs;

    if (isVectorNull(clent->vAngles)) {
        mins = clent->vOrigin + model->vMins;
        maxs = clent->vOrigin + model->vMaxs;
    } else {
        glm::vec3 radius = glm::vec3(model->flRadius);
        mins = clent->vOrigin - radius;
        maxs = clent->vOrigin + radius;
    }

    if (context.cullBox(mins, maxs)) {
        return true;
    }

    entData.transformMat = getBrushTransform(clent);
    return false;
}

glm::mat4 SceneRenderer::BrushRenderer::getBrushTransform(const ClientEntity *clent) const {
    glm::mat4 modelMat = glm::identity<glm::mat4>();
    modelMat = glm::rotate(modelMat, glm::radians(clent->vAngles.z), {1.0f, 0.0f, 0.0f});
    modelMat = glm::rotate(modelMat, glm::radians(clent->vAngles.x), {0.0f, 1.0f, 0.0f});
    modelMat = glm::rotate(modelMat, glm::radians(clent->vAngles.y), {0.0f, 0.0f, 1.0f});
    modelMat = glm::translate(modelMat, clent->vOrigin);
    return modelMat;
}

void SceneRenderer::BrushRenderer::batchAddSurface(const Surface &surf) {
    if (m_uWorldEboIdx + surf.vertexCount >= m_WorldEboBuf.size()) {
        // EBO overflow
        batchFlush();
        batchReset();
        m_Renderer.m_Stats.uEboOverflows++;
    }

    for (uint16_t j = 0; j < surf.vertexCount; j++) {
        m_WorldEboBuf[m_uWorldEboIdx + j] = (uint16_t)surf.vertexOffset + j;
    }

    m_WorldEboBuf[m_uWorldEboIdx + surf.vertexCount] = PRIMITIVE_RESTART_IDX;
    m_uWorldEboIdx += surf.vertexCount + 1;
}

void SceneRenderer::BrushRenderer::batchFlush() {
    if (m_uWorldEboOffset == m_uWorldEboIdx) {
        return;
    }

    // Decrement EBO size to remove last PRIMITIVE_RESTART_IDX
    m_uWorldEboIdx--;

    // Update EBO
    m_WorldEbo.update(m_uWorldEboOffset * sizeof(uint16_t),
                      (m_uWorldEboIdx - m_uWorldEboOffset) * sizeof(uint16_t),
                      m_WorldEboBuf.data() + m_uWorldEboOffset);

    // Draw elements
    glDrawElements(GL_TRIANGLE_FAN, (m_uWorldEboIdx - m_uWorldEboOffset), GL_UNSIGNED_SHORT,
                   reinterpret_cast<const void *>(m_uWorldEboOffset * sizeof(uint16_t)));
    m_Renderer.m_Stats.uDrawCalls++;

    m_uWorldEboOffset = m_uWorldEboIdx;
}

void SceneRenderer::BrushRenderer::batchReset() {
    m_uWorldEboIdx = 0;
    m_uWorldEboOffset = 0;
    m_pBatchMaterial = nullptr;
}
