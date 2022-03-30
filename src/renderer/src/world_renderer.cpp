#include <renderer/utils.h>
#include <renderer/scene_shaders.h>
#include "world_renderer.h"

ConVar<bool> r_lockpvs("r_lockpvs", false, "Lock current PVS to let devs see where it ends");
ConVar<bool> r_novis("r_novis", false, "Ignore visibility data");

SceneRenderer::WorldRenderer::WorldRenderer(SceneRenderer &renderer)
	: m_Renderer(renderer) {
    createLeaves();
    createNodes();
    updateNodeParents(0, 0);
    createBuffers();
}

void SceneRenderer::WorldRenderer::getTexturedWorldSurfaces(ViewContext &context,
                                                            WorldSurfaceList &surfList) const {
    if (surfList.visFrame == 0) {
        surfList.nodeVisFrame.resize(m_Nodes.size());
        surfList.leafVisFrame.resize(m_Leaves.size());
        surfList.visFrame++;
    }

    // Prepare surf list
    surfList.textureChainFrame++;
    surfList.textureChain.resize(m_Renderer.m_uNextMaterialIndex);
    surfList.textureChainFrames.resize(m_Renderer.m_uNextMaterialIndex);
    surfList.skySurfaces.clear();

    markLeaves(context, surfList);
    recursiveWorldNodesTextured(context, surfList, 0);
}

void SceneRenderer::WorldRenderer::drawTexturedWorld(WorldSurfaceList &surfList) {
    auto &textureChain = surfList.textureChain;
    auto &textureChainFrames = surfList.textureChainFrames;
    unsigned frame = surfList.textureChainFrame;
    unsigned drawnSurfs = 0;

    // Set up GL
    glBindVertexArray(m_Renderer.m_SurfaceVao);
    m_WorldEbo.bind();

    // Draw texture chains
    unsigned eboIdx = 0;

    for (size_t i = 0; i < textureChain.size(); i++) {
        if (textureChainFrames[i] != frame) {
            continue;
        }

        // Bind material
        const Material *mat = m_Renderer.m_Surfaces[textureChain[i][0]].material;
        mat->activateTextures();
        mat->enableShader(SHADER_TYPE_WORLD_IDX, m_Renderer.m_uFrameCount);

        // Fill the EBO
        unsigned eboOffset = eboIdx;
        unsigned vertexCount = 0;
        for (unsigned surfIdx : textureChain[i]) {
            Surface &surf = m_Renderer.m_Surfaces[surfIdx];
            uint16_t vertCount = (uint16_t)surf.vertexCount;

            for (uint16_t j = 0; j < vertCount; j++) {
                m_WorldEboBuf[eboIdx + j] = (uint16_t)surf.vertexOffset + j;
            }

            eboIdx += vertCount;
            m_WorldEboBuf[eboIdx] = PRIMITIVE_RESTART_IDX;
            eboIdx++;
            vertexCount += vertCount + 1;
        }

        // Decrement EBO size to remove last PRIMITIVE_RESTART_IDX
        eboIdx--;
        vertexCount--;

        AFW_ASSERT(eboIdx - eboOffset == vertexCount);
        AFW_ASSERT(eboOffset + vertexCount < m_WorldEboBuf.size());

        // Update EBO
        m_WorldEbo.update(eboOffset * sizeof(uint16_t), vertexCount * sizeof(uint16_t),
                          m_WorldEboBuf.data() + eboOffset);

        // Draw elements
        glDrawElements(GL_TRIANGLE_FAN, vertexCount, GL_UNSIGNED_SHORT,
                       reinterpret_cast<const void *>(eboOffset * sizeof(uint16_t)));

        drawnSurfs += (unsigned)textureChain[i].size();
        m_Renderer.m_Stats.uDrawCalls++;
    }

    m_Renderer.m_Stats.uWorldPolys += drawnSurfs;
}

void SceneRenderer::WorldRenderer::drawSkybox(WorldSurfaceList &surfList) {
    AFW_ASSERT(m_Renderer.m_pSkyboxMaterial);

    if (surfList.skySurfaces.empty()) {
        return;
    }

    // Set up GL
    glBindVertexArray(m_Renderer.m_SurfaceVao);
    m_WorldEbo.bind();
    glDepthFunc(GL_LEQUAL);

    // Bind material
    const Material *mat = m_Renderer.m_pSkyboxMaterial;
    mat->activateTextures();
    mat->enableShader(SHADER_TYPE_WORLD_IDX, m_Renderer.m_uFrameCount);

    // Fill the EBO
    unsigned eboIdx = 0;
    for (unsigned surfIdx : surfList.skySurfaces) {
        Surface &surf = m_Renderer.m_Surfaces[surfIdx];
        uint16_t vertCount = (uint16_t)surf.vertexCount;

        for (uint16_t j = 0; j < vertCount; j++) {
            m_WorldEboBuf[eboIdx + j] = (uint16_t)surf.vertexOffset + j;
        }

        eboIdx += vertCount;
        m_WorldEboBuf[eboIdx] = PRIMITIVE_RESTART_IDX;
        eboIdx++;
    }

    // Decrement EBO size to remove last PRIMITIVE_RESTART_IDX
    eboIdx--;

    // Update EBO
    m_WorldEbo.update(0, eboIdx * sizeof(uint16_t), m_WorldEboBuf.data());

    // Draw elements
    glDrawElements(GL_TRIANGLE_FAN, eboIdx, GL_UNSIGNED_SHORT, nullptr);

    m_Renderer.m_Stats.uSkyPolys += (unsigned)surfList.skySurfaces.size();
    m_Renderer.m_Stats.uDrawCalls++;

    // Restore GL
    glDepthFunc(GL_LESS);
}

void SceneRenderer::WorldRenderer::drawWireframe(WorldSurfaceList &surfList, bool drawSky) {
    auto &textureChain = surfList.textureChain;
    auto &textureChainFrames = surfList.textureChainFrames;
    unsigned frame = surfList.textureChainFrame;
    
    // Fill the EBO with world surfaces
    unsigned eboIdx = 0;

    for (size_t i = 0; i < textureChain.size(); i++) {
        if (textureChainFrames[i] != frame) {
            continue;
        }

        for (unsigned surfIdx : textureChain[i]) {
            Surface &surf = m_Renderer.m_Surfaces[surfIdx];
            uint16_t vertCount = (uint16_t)surf.vertexCount;

            for (uint16_t j = 0; j < vertCount; j++) {
                m_WorldEboBuf[eboIdx + j] = (uint16_t)surf.vertexOffset + j;
            }

            eboIdx += vertCount;
            m_WorldEboBuf[eboIdx] = PRIMITIVE_RESTART_IDX;
            eboIdx++;
        }
    }

    // Fill the EBO with sky surfaces
    if (drawSky) {
        for (unsigned surfIdx : surfList.skySurfaces) {
            Surface &surf = m_Renderer.m_Surfaces[surfIdx];
            uint16_t vertCount = (uint16_t)surf.vertexCount;

            for (uint16_t j = 0; j < vertCount; j++) {
                m_WorldEboBuf[eboIdx + j] = (uint16_t)surf.vertexOffset + j;
            }

            eboIdx += vertCount;
            m_WorldEboBuf[eboIdx] = PRIMITIVE_RESTART_IDX;
            eboIdx++;
        }
    }

    if (eboIdx == 0) {
        return;
    }

    // Set up GL
    glBindVertexArray(m_Renderer.m_SurfaceVao);
    m_WorldEbo.bind();

    // Enable shader
    ShaderInstance *shaderInstance =
        SceneShaders::Shaders::brushWireframe.getShaderInstance(SHADER_TYPE_CUSTOM_IDX);
    shaderInstance->enable(m_Renderer.m_uFrameCount);
    shaderInstance->getShader<SceneShaders::BrushWireframeShader>().setColor(glm::vec3(0.8f));

    // Decrement EBO size to remove last PRIMITIVE_RESTART_IDX
    eboIdx--;

    // Update EBO
    m_WorldEbo.update(0, eboIdx * sizeof(uint16_t), m_WorldEboBuf.data());

    // Draw elements
    glDrawElements(GL_LINE_LOOP, eboIdx, GL_UNSIGNED_SHORT, nullptr);
    m_Renderer.m_Stats.uDrawCalls++;
}

void SceneRenderer::WorldRenderer::createLeaves() {
    auto &lvlLeaves = m_Renderer.m_Level.getLeaves();
    auto &lvlVisData = m_Renderer.m_Level.getVisData();
    m_Leaves.resize(lvlLeaves.size());

    for (size_t i = 0; i < lvlLeaves.size(); i++) {
        const bsp::BSPLeaf &lvlLeaf = lvlLeaves[i];
        Leaf &leaf = m_Leaves[i];

        leaf.nContents = lvlLeaf.nContents;

        if (lvlLeaf.nVisOffset != -1 && !lvlVisData.empty()) {
            leaf.pCompressedVis = &lvlVisData.at(lvlLeaf.nVisOffset);
        }
    }
}

void SceneRenderer::WorldRenderer::createNodes() {
    auto &lvlNodes = m_Renderer.m_Level.getNodes();
    auto &lvlPlanes = m_Renderer.m_Level.getPlanes();
    m_Nodes.resize(lvlNodes.size());

	for (size_t i = 0; i < lvlNodes.size(); i++) {
        const bsp::BSPNode &lvlNode = lvlNodes[i];
        Node &node = m_Nodes[i];

        node.iFirstSurface = lvlNode.firstFace;
        node.iNumSurfaces = lvlNode.nFaces;
        node.pPlane = &lvlPlanes.at(lvlNode.iPlane);
        node.iChildren[0] = lvlNode.iChildren[0];
        node.iChildren[1] = lvlNode.iChildren[1];
    }
}

void SceneRenderer::WorldRenderer::updateNodeParents(int iNode, int parent) {
    if (iNode >= 0) {
        Node &node = m_Nodes[iNode];
        node.iParentIdx = parent;
        updateNodeParents(node.iChildren[0], iNode);
        updateNodeParents(node.iChildren[1], iNode);
    } else {
        Leaf &leaf = m_Leaves[~iNode];
        leaf.iParentIdx = parent;
    }
}

void SceneRenderer::WorldRenderer::createBuffers() {
    // Maximum ebo size is X vertices + Y primitive restarts.
    // X = max vertex count, Y = max surface count.
    unsigned maxEboSize = m_Renderer.m_uSurfaceVertexBufferSize + (unsigned)m_Renderer.m_Surfaces.size();
    m_WorldEboBuf.resize(maxEboSize);
    m_WorldEbo.create(GL_ELEMENT_ARRAY_BUFFER, "WorldRenderer: EBO");
    m_WorldEbo.init(maxEboSize * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);
    printi("WorldRenderer: EBO is {:.1f} KiB", m_WorldEbo.getMemUsage() / 1024.0);
}

void SceneRenderer::WorldRenderer::markLeaves(ViewContext &context,
                                              WorldSurfaceList &surfList) const {
    if (r_lockpvs.getValue()) {
        // PVS update is disabled
        return;
    }

    int viewleaf = m_Renderer.m_Level.pointInLeaf(context.getViewOrigin());

    if (viewleaf == surfList.viewLeaf) {
        // Leaf not changed, keep PVS as is
        return;
    }

    surfList.viewLeaf = viewleaf;
    surfList.visFrame++;
    unsigned visFrame = surfList.visFrame;

    uint8_t visBuf[bsp::MAX_MAP_LEAFS / 8];
    const uint8_t *vis = nullptr;

    if (r_novis.getValue()) {
        AFW_ASSERT_REL(((m_Leaves.size() + 7) >> 3) <= sizeof(visBuf));
        memset(visBuf, 0xFF, (m_Leaves.size() + 7) >> 3);
        vis = visBuf;
    } else {
        vis = m_Renderer.m_Level.leafPVS(viewleaf, visBuf);
    }

    // Node 0 is always visible
    surfList.nodeVisFrame[0] = visFrame;

    for (unsigned i = 0; i < m_Leaves.size() - 1; i++) {
        if (vis[i >> 3] & (1 << (i & 7))) {
            int node = ~(i + 1);

            do {
                if (node < 0) {
                    // Leaf
                    if (surfList.leafVisFrame[~node] == visFrame) {
                        break;
                    }

                    surfList.leafVisFrame[~node] = visFrame;
                    node = m_Leaves[~node].iParentIdx;
                } else {
                    // Node
                    if (surfList.nodeVisFrame[node] == visFrame) {
                        break;
                    }

                    surfList.nodeVisFrame[node] = visFrame;
                    node = m_Nodes[node].iParentIdx;
                }
            } while (node);
        }
    }
}

void SceneRenderer::WorldRenderer::recursiveWorldNodesTextured(ViewContext &context,
                                                               WorldSurfaceList &surfList,
                                                               int nodeIdx) const {
    if (nodeIdx < 0) {
        // Leaf
        return;
    }

    if (surfList.nodeVisFrame[nodeIdx] != surfList.visFrame) {
        // Not in PVS
        return;
    }

    const Node &node = m_Nodes[nodeIdx];

    float dot = planeDiff(context.getViewOrigin(), *node.pPlane);
    int side = (dot >= 0.0f) ? 0 : 1;

    // Front side
    recursiveWorldNodesTextured(context, surfList, node.iChildren[side]);

    // Draw surfaces of current node
    for (unsigned i = 0; i < node.iNumSurfaces; i++) {
        unsigned idx = node.iFirstSurface + i;
        Surface &surf = m_Renderer.m_Surfaces[idx];

        if (context.cullSurface(surf)) {
            continue;
        }

        if (surf.flags & SURF_DRAWSKY) {
            surfList.skySurfaces.push_back(idx);
        } else {
            unsigned matIdx = surf.materialIdx;
            unsigned oldChainFrame = surfList.textureChainFrames[matIdx];
            if (oldChainFrame != surfList.textureChainFrame) {
                surfList.textureChainFrames[matIdx] = surfList.textureChainFrame;
                surfList.textureChain[matIdx].clear();
            }

            surfList.textureChain[matIdx].push_back(idx);
        }
    }

    // Back side
    recursiveWorldNodesTextured(context, surfList, node.iChildren[!side]);
}
