#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <appfw/prof.h>
#include <app_base/lightmap.h>
#include <stb_image.h>
#include <material_system/shader_instance.h>
#include <renderer/scene_renderer.h>
#include <renderer/renderer_engine_interface.h>
#include <imgui.h>
#include <gui_app_base/imgui_controls.h>
#include "scene_shaders.h"

ConVar<int> r_cull("r_cull", 1,
                   "Backface culling:\n"
                   "0 - none\n"
                   "1 - back\n"
                   "2 - front",
                   [](const int &, const int &newVal) {
                       if (newVal < 0 || newVal > 2)
                           return false;
                       return true;
                   });

ConVar<bool> r_drawworld("r_drawworld", true, "Draw world polygons");
ConVar<bool> r_drawsky("r_drawsky", true, "Draw skybox");
ConVar<bool> r_drawents("r_drawents", true, "Draw entities");
ConVar<float> r_gamma("r_gamma", 2.2f, "Gamma");
ConVar<float> r_texgamma("r_texgamma", 2.2f, "Texture gamma");
ConVar<int> r_texture("r_texture", 2,
              "Which texture should be used:\n  0 - white color, 1 - random color, 2 - texture");
ConVar<int> r_shading("r_shading", 2, "Lighting method:\n  0 - fullbright, 1 - shaded, 2 - lightmaps");
ConVar<int> r_lightmap("r_lightmap", 1, "Used lightmap:\n  0 - BSP, 1 - custom");
ConVar<bool> r_ebo("r_ebo", true, "Use indexed rendering");
ConVar<bool> r_wireframe("r_wireframe", false, "Draw wireframes");
ConVar<bool> r_nosort("r_nosort", false, "Disable transparent entity sorting");
ConVar<bool> r_notrans("r_notrans", false, "Disable entity transparency");
ConVar<bool> r_filter_lm("r_filter_lm", true, "Filter lightmaps (requires map reload)");
ConVar<bool> r_patches("r_patches", false, "Draw custom lightmap patches");

static const char *r_shading_values[] = {"Fullbright", "Shaded", "Lightmaps"};
static const char *r_lightmap_values[] = {"BSP", "Custom"};

static inline bool isVectorNull(const glm::vec3 &v) { return v.x == 0.f && v.y == 0.f && v.z == 0.f; }

//----------------------------------------------------------------
// SceneRenderer
//----------------------------------------------------------------
SceneRenderer::SceneRenderer() {
    createScreenQuad();
    createGlobalUniform();

    m_SolidEntityList.reserve(MAX_VISIBLE_ENTS);
    m_SortBuffer.reserve(MAX_TRANS_SURFS_PER_MODEL);

    m_pSkyboxMaterial = MaterialSystem::get().createMaterial("Skybox");
    m_pSkyboxMaterial->setSize(1, 1);
    m_pSkyboxMaterial->setUsesGraphicalSettings(true);
    m_pSkyboxMaterial->setShader(SHADER_TYPE_WORLD_IDX, &Shaders::skybox);
}

SceneRenderer::~SceneRenderer() {
    MaterialSystem::get().destroyMaterial(m_pSkyboxMaterial);
    m_pSkyboxMaterial = nullptr;
}

void SceneRenderer::beginLoading(const bsp::Level *level, std::string_view path) {
    AFW_ASSERT(level);

    if (m_pLevel == level) {
        return;
    }

    unloadLevel();

    m_Data.customLightmapPath = getFileSystem().findExistingFile(std::string(path) + ".lm", std::nothrow);

    m_pLevel = level;
    m_Surf.setLevel(m_pLevel);
    loadTextures();
    loadSkyBox();

    m_pLoadingState = std::make_unique<LoadingState>();
    m_LoadingStatus = LoadingStatus::CreateSurfaces;
    m_pLoadingState->createSurfacesResult = std::async(std::launch::async, [this]() { asyncCreateSurfaces(); });
}

void SceneRenderer::optimizeBrushModel(Model *model) {
    AFW_ASSERT(model->getType() == ModelType::Brush);
    OptBrushModel om;

    // Create a list of all surfaces of the brush
    om.surfs.reserve(model->getFaceNum());
    unsigned last = model->getFirstFace() + model->getFaceNum();
    for (unsigned surfidx = model->getFirstFace(); surfidx < last; surfidx++) {
        om.surfs.push_back(surfidx);
    }

    // Sort the list by material
    std::sort(om.surfs.begin(), om.surfs.end(),
              [this](const unsigned &lhsidx, const unsigned &rhsidx) {
                  auto &lhs = m_Surf.getSurface(lhsidx);
                  auto &rhs = m_Surf.getSurface(rhsidx);
                  return lhs.uMaterialIdx < rhs.uMaterialIdx;
              });

    model->setOptModelIdx((unsigned)m_Data.optBrushModels.size());
    m_Data.optBrushModels.push_back(std::move(om));
}

void SceneRenderer::unloadLevel() {
    AFW_ASSERT(!isLoading());

    if (isLoading()) {
        throw std::logic_error("can't unload level while loading");
    }

    if (m_pLevel) {
        m_pLoadingState = nullptr;
        m_Data = LevelData();
        m_Surf.setLevel(nullptr);
        m_pLevel = nullptr;
    }
}

bool SceneRenderer::loadingTick() { 
    AFW_ASSERT(m_pLoadingState);

    try {
        switch (m_LoadingStatus) {
        case LoadingStatus::CreateSurfaces: {
            if (appfw::isFutureReady(m_pLoadingState->createSurfacesResult)) {
                m_pLoadingState->createSurfacesResult.get();
                m_pLoadingState->createSurfacesResult = std::future<void>();

                // Start async tasks
                m_LoadingStatus = LoadingStatus::AsyncTasks;

                m_pLoadingState->loadBSPLightmapsResult =
                    std::async(std::launch::async, [this]() { asyncLoadBSPLightmaps(); });

                m_pLoadingState->loadCustomLightmapsResult =
                    std::async(std::launch::async, [this]() { asyncLoadCustomLightmaps(); });
            }
            return false;
        }
        case LoadingStatus::AsyncTasks: {
            if (!m_pLoadingState->loadBSPLightmapsFinished) {
                if (appfw::isFutureReady(m_pLoadingState->loadBSPLightmapsResult)) {
                    m_pLoadingState->loadBSPLightmapsResult.get();
                    m_pLoadingState->loadBSPLightmapsResult = std::future<void>();
                    finishLoadBSPLightmaps();
                    m_pLoadingState->loadBSPLightmapsFinished = true;
                }
            }

            if (!m_pLoadingState->loadCustomLightmapsFinished) {
                if (appfw::isFutureReady(m_pLoadingState->loadCustomLightmapsResult)) {
                    m_pLoadingState->loadCustomLightmapsResult.get();
                    m_pLoadingState->loadCustomLightmapsResult = std::future<void>();
                    finishLoadCustomLightmaps();
                    m_pLoadingState->loadCustomLightmapsFinished = true;
                }
            }

            bool isReady = m_pLoadingState->loadBSPLightmapsFinished && m_pLoadingState->loadCustomLightmapsFinished;

            if (isReady) {
                m_LoadingStatus = LoadingStatus::CreateSurfaceObjects;
                m_pLoadingState->createSurfaceObjectsResult =
                    std::async(std::launch::async, [this]() { asyncCreateSurfaceObjects(); });
            }

            return false;
        }
        case LoadingStatus::CreateSurfaceObjects: {
            if (appfw::isFutureReady(m_pLoadingState->createSurfaceObjectsResult)) {
                m_pLoadingState->createSurfaceObjectsResult.get();
                m_pLoadingState->createSurfaceObjectsResult = std::future<void>();
                finishCreateSurfaceObjects();
                finishLoading();
                return true;
            }
            return false;
        }
        }
    }
    catch (const std::exception &) {
        m_pLoadingState = nullptr;
        unloadLevel();
        throw;
    }

    AFW_ASSERT(false);
    return false;
}

void SceneRenderer::setPerspective(float fov, float aspect, float near, float far) {
    m_Data.viewContext.setPerspective(fov, aspect, near, far);
}

void SceneRenderer::setPerspViewOrigin(const glm::vec3 &origin, const glm::vec3 &angles) {
    m_Data.viewContext.setPerspViewOrigin(origin, angles);
}

void SceneRenderer::setViewportSize(const glm::ivec2 &size) {
    m_vViewportSize = size;
    m_bNeedRefreshFB = true;
}

void SceneRenderer::renderScene(GLint targetFb, float flSimTime, float flTimeDelta) {
    appfw::Timer frameTimer;
    frameTimer.start();

    AFW_ASSERT(m_pLevel);
    AFW_ASSERT(!isLoading());

    appfw::Prof mainprof("Render Scene");

    // Reset stats
    m_Stats = RenderingStats();

    LightmapType newLmType = (LightmapType)std::clamp(r_lightmap.getValue(), 0, 1);

    if (newLmType == LightmapType::Custom && !m_Data.bCustomLMLoaded) {
        r_lightmap.setValue(0);
        newLmType = LightmapType::BSP;
        printw("Lightmap type set to BSP since custom lightmaps are not loaded");
    }
    
    if (newLmType == LightmapType::BSP == 0 && m_pLevel->getLightMaps().size() == 0 && r_shading.getValue() == 2) {
        r_shading.setValue(1);
        printw("Lighting type set to shaded since no lightmaps are loaded");
    }

    if (m_Data.lightmapType != newLmType) {
        m_Data.lightmapType = newLmType;
        updateVao();
    }

    frameSetup(flSimTime, flTimeDelta);

    if (r_drawworld.getValue()) {
        drawWorldSurfaces();

        if (r_drawsky.getValue()) {
            drawSkySurfaces();
        }
    }

    if (r_drawents.getValue()) {
        appfw::Prof prof("Entities");
        drawSolidEntities();
        drawSolidTriangles();
        drawTransEntities();
        drawTransTriangles();
    }

    // Draw patches
    if (m_Data.patchesVao.id() != 0 && r_patches.getValue()) {
        drawPatches();
    }

    frameEnd();

    glBindFramebuffer(GL_FRAMEBUFFER, targetFb);
    doPostProcessing();

    frameTimer.stop();
    m_Stats.flFrameTime = frameTimer.dseconds();
}

void SceneRenderer::showDebugDialog(const char *title, bool *isVisible) {
    if (!isVisible || *isVisible) {
        ImGui::SetNextWindowBgAlpha(0.2f);

        if (!ImGui::Begin(title, isVisible)) {
            ImGui::End();
            return;
        }

        ImGui::Text("FPS: %.3f", 1.0 / m_Stats.flFrameTime);
        ImGui::Text("Total: %.3f ms", m_Stats.flFrameTime * 1000);
        ImGui::Separator();

        ImGui::Text("World: %u (%u + %u)", m_Stats.uRenderedWorldPolys + m_Stats.uRenderedSkyPolys,
                    m_Stats.uRenderedWorldPolys, m_Stats.uRenderedSkyPolys);
        ImGui::Text("Brush ent surfs: %u", m_Stats.uRenderedBrushEntPolys);
        ImGui::Text("Draw calls: %u", m_Stats.uDrawCallCount);
        ImGui::Separator();

        ImGui::Text("Entities: %u", m_uVisibleEntCount);
        ImGui::Text("    Solid: %lu", m_SolidEntityList.size());
        ImGui::Text("    Trans: %lu", m_TransEntityList.size());
        ImGui::Separator();

        CvarCheckbox("World", r_drawworld);
        ImGui::SameLine();
        CvarCheckbox("Sky", r_drawsky);
        ImGui::SameLine();
        CvarCheckbox("EBO", r_ebo);

        CvarCheckbox("Entities", r_drawents);
        ImGui::SameLine();
        CvarCheckbox("Patches", r_patches);

        int lighting = std::clamp(r_shading.getValue(), 0, 2);

        if (ImGui::BeginCombo("Shading", r_shading_values[lighting])) {
            for (int i = 0; i <= 2; i++) {
                if (ImGui::Selectable(r_shading_values[i])) {
                    r_shading.setValue(i);
                }
                if (lighting == i) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        int lightmap = std::clamp(r_lightmap.getValue(), 0, 2);

        if (ImGui::BeginCombo("Lightmaps", r_lightmap_values[lightmap])) {
            for (int i = 0; i <= 1; i++) {
                if (ImGui::Selectable(r_lightmap_values[i])) {
                    r_lightmap.setValue(i);
                }
                if (lightmap == i) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Reload lightmaps")) {
            reloadCustomLightmaps();
        }

        ImGui::End();
    }
}

void SceneRenderer::clearEntities() {
    m_SolidEntityList.clear();
    m_TransEntityList.clear();
    m_uVisibleEntCount = 0;
}

bool SceneRenderer::addEntity(ClientEntity *pClent) {
    if (m_uVisibleEntCount == MAX_VISIBLE_ENTS) {
        return false;
    }

    if (pClent->isOpaque()) {
        // Solid
        m_SolidEntityList.push_back(pClent);
    } else {
        // Transparent
        m_TransEntityList.push_back(pClent);
    }
    m_uVisibleEntCount++;
    return true;
}

Material *SceneRenderer::getSurfaceMaterial(int surface) {
    return m_Data.surfaces[surface].m_pMat;
}

void SceneRenderer::reloadCustomLightmaps() {
    AFW_ASSERT_REL(!m_pLoadingState);
    m_pLoadingState = std::make_unique<LoadingState>();
    asyncLoadCustomLightmaps();
    finishLoadCustomLightmaps();
    m_pLoadingState = nullptr;
}

#ifdef RENDERER_SUPPORT_TINTING
void SceneRenderer::setSurfaceTint(int surface, glm::vec4 color) {
    Surface &surf = m_Data.surfaces[surface];
    color.r = pow(color.r, 2.2f);
    color.g = pow(color.g, 2.2f);
    color.b = pow(color.b, 2.2f);
    std::vector<glm::vec4> colors(surf.m_iVertexCount, color);
    m_Data.surfTintBuf.bind();
    m_Data.surfTintBuf.update(sizeof(glm::vec4) * surf.m_nFirstVertex,
                              sizeof(glm::vec4) * colors.size(), colors.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
Shader *SceneRenderer::getDefaultSurfaceShader() {
    return &Shaders::world;
}
#endif

void SceneRenderer::createScreenQuad() {
    // clang-format off
    const float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // clang-format on

    m_nQuadVao.create();
    m_nQuadVbo.create(GL_ARRAY_BUFFER, "SceneRenderer: Quad VBO");

    glBindVertexArray(m_nQuadVao);
    m_nQuadVbo.bind();
    m_nQuadVbo.init(sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glBindVertexArray(0);
}

void SceneRenderer::createGlobalUniform() {
    m_GlobalUniformBuffer.create(GL_UNIFORM_BUFFER, "SceneRenderer: Global Uniform");
    m_GlobalUniformBuffer.bind();
    m_GlobalUniformBuffer.init(sizeof(GlobalUniform), nullptr, GL_DYNAMIC_DRAW);
    m_GlobalUniformBuffer.unbind();
}

void SceneRenderer::recreateFramebuffer() {
    destroyFramebuffer();

    // Create FP color buffer
    m_nColorBuffer.create("SceneRenderer: Backbuffer color buffer");
    m_nColorBuffer.initTexture(GraphicsFormat::RGBA16F, m_vViewportSize.x, m_vViewportSize.y, false,
                               GL_RGBA, GL_FLOAT, nullptr);

    // Create depth buffer
    m_nRenderBuffer.create("SceneRenderer: Backbuffer depth buffer");
    m_nRenderBuffer.init(GraphicsFormat::Depth32, m_vViewportSize.x, m_vViewportSize.y);

    // Attach buffers
    m_nHdrFramebuffer.create();
    glBindFramebuffer(GL_FRAMEBUFFER, m_nHdrFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_nColorBuffer.getId(), 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_nRenderBuffer.getId());
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("SceneRenderer::recreateFramebuffer(): HDR framebuffer not complete");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneRenderer::destroyFramebuffer() {
    m_nHdrFramebuffer.destroy();
    m_nColorBuffer.destroy();
    m_nRenderBuffer.destroy();
}

void SceneRenderer::asyncCreateSurfaces() {
    printi("Creating surfaces...");

    m_Data.surfaces.resize(m_Surf.getSurfaceCount());

    for (size_t i = 0; i < m_Data.surfaces.size(); i++) {
        Surface &surf = m_Data.surfaces[i];
        const SurfaceRenderer::Surface &baseSurf = m_Surf.getSurface(i);
        
        surf.m_pMat = baseSurf.pMaterial;
        surf.m_iVertexCount = (GLsizei)baseSurf.vVertices.size();

        auto fnGetRandColor = []() { return (rand() % 256) / 255.f; };
        surf.m_Color = {fnGetRandColor(), fnGetRandColor(), fnGetRandColor()};
    }
}

void SceneRenderer::asyncLoadBSPLightmaps() {
    if (m_pLevel->getLightMaps().size() == 0) {
        printi("BSP lightmaps: no lightmaps in the map file");
        return;
    }

    printi("BSP lightmaps: loading...");
    m_pLoadingState->bspLightmapBlock.resize(BSP_LIGHTMAP_BLOCK_SIZE, BSP_LIGHTMAP_BLOCK_SIZE);

    struct Lightmap {
        unsigned surface;
        int size;
        uint32_t offset;

        inline bool operator<(const Lightmap &rhs) const {
            return size > rhs.size;
        }
    };

    std::vector<Lightmap> sortedLightmaps;
    sortedLightmaps.reserve(m_Data.surfaces.size());

    for (size_t i = 0; i < m_Data.surfaces.size(); i++) {
        Surface &surf = m_Data.surfaces[i];
        const SurfaceRenderer::Surface &baseSurf = m_Surf.getSurface(i);

        // Calculate texture extents
        glm::vec2 mins = {999999, 999999};
        glm::vec2 maxs = {-99999, -99999};

        for (size_t j = 0; j < baseSurf.vVertices.size(); j++) {
            glm::vec3 vert = baseSurf.vVertices[j];

            float vals = glm::dot(vert, baseSurf.pTexInfo->vS) + baseSurf.pTexInfo->fSShift;
            mins.s = std::min(mins.s, vals);
            maxs.s = std::max(maxs.s, vals);

            float valt = glm::dot(vert, baseSurf.pTexInfo->vT) + baseSurf.pTexInfo->fTShift;
            mins.t = std::min(mins.t, valt);
            maxs.t = std::max(maxs.t, valt);
        }
        
        // Calculate lightmap texture extents and size
        glm::vec2 texmins = {floor(mins.x / BSP_LIGHTMAP_DIVISOR), floor(mins.y / BSP_LIGHTMAP_DIVISOR)};
        glm::vec2 texmaxs = {ceil(maxs.x / BSP_LIGHTMAP_DIVISOR), ceil(maxs.y / BSP_LIGHTMAP_DIVISOR)};
        surf.m_vTextureMins = texmins * (float)BSP_LIGHTMAP_DIVISOR; // This one used in the engine (I think)
        //surf.m_vTextureMins = mins; // This one makes more sense. I don't see any difference in-game between them though

        int wide = (int)((texmaxs - texmins).x + 1);
        int tall = (int)((texmaxs - texmins).y + 1);
        surf.m_BSPLMSize = {wide, tall};

        // Validate size
        uint32_t offset = baseSurf.nLightmapOffset;

        if (offset == bsp::NO_LIGHTMAP_OFFSET) {
            continue;
        }

        if (offset + wide * tall * 3 > m_pLevel->getLightMaps().size()) {
            printw("BSP lightmaps: face {} has invalid lightmap offset", i);
            continue;
        }

        // Combine all lightstyles
        // TODO: need only combine what is needed
        /*
        uint32_t lmBytesSize = wide * tall * 3;
        const bsp::BSPFace &face = m_pLevel->getFaces()[i];
        std::vector<uint8_t> lm(lmBytesSize);
        for (int j = 0; j < bsp::NUM_LIGHTSTYLES && face.nStyles[j] != 255; j++) {
            for (int k = 0; k < lm.size(); k++) {
                lm[k] += m_pLevel->getLightMaps()[offset + lmBytesSize * j + k];
            }
        }
        */

        // Add to sorting list
        sortedLightmaps.push_back({(unsigned)i, wide * tall, offset});
    }

    // Sort all lightmaps by size, large to small
    std::sort(sortedLightmaps.begin(), sortedLightmaps.end());

    // Add them to the lightmap block
    appfw::Timer timer;
    timer.start();

    for (auto &i : sortedLightmaps) {
        Surface &surf = m_Data.surfaces[i.surface];
        const glm::u8vec3 *data = reinterpret_cast<const glm::u8vec3 *>(m_pLevel->getLightMaps().data() + i.offset);
        int x, y;
        
        if (m_pLoadingState->bspLightmapBlock.insert(data, surf.m_BSPLMSize.x, surf.m_BSPLMSize.y, x, y,
                                                     BSP_LIGHTMAP_PADDING)) {
            surf.m_BSPLMOffset.x = x;
            surf.m_BSPLMOffset.y = y;
        } else {
            printw("BSP lightmaps: no space for surface {} ({}x{})", i.surface, surf.m_BSPLMSize.x,
                    surf.m_BSPLMSize.y);
        }
    }

    timer.stop();
    printi("BSP lightmaps: block {:.3f} s", timer.dseconds());
}

void SceneRenderer::finishLoadBSPLightmaps() {
    // Load the block into the GPU
    Texture2D &texture = m_Data.bspLightmapBlockTex;
    TextureFilter filter =
        r_filter_lm.getValue() ? TextureFilter::Bilinear : TextureFilter::Nearest;
    texture.create("SceneRenderer: BSP lightmap block");
    texture.setWrapMode(TextureWrapMode::Clamp);
    texture.setFilter(filter);
    texture.initTexture(GraphicsFormat::RGB8, BSP_LIGHTMAP_BLOCK_SIZE, BSP_LIGHTMAP_BLOCK_SIZE,
                        false, GL_RGB, GL_UNSIGNED_BYTE,
                        m_pLoadingState->bspLightmapBlock.getData());

    m_pLoadingState->bspLightmapBlock.clear();
    printi("BSP lightmaps: loaded.");
}

void SceneRenderer::asyncLoadCustomLightmaps() {
    m_Data.bCustomLMLoaded = false;
    fs::path &lmPath = m_Data.customLightmapPath;

    if (lmPath.empty()) {
        printi("Custom lightmaps: file not found - not loading.");
        return;
    }

    printi("Custom lightmaps: loading...");

    try {
        appfw::BinaryInputFile file(lmPath);

        // Magic
        uint8_t magic[sizeof(LightmapFileFormat::MAGIC)];
        file.readBytes(magic, sizeof(magic));

        if (memcmp(magic, LightmapFileFormat::MAGIC, sizeof(magic))) {
            throw std::runtime_error("Invalid magic");
        }

        // Level & lightmap info
        uint32_t faceCount = file.readUInt32(); // Face count

        if (faceCount != m_Data.surfaces.size()) {
            throw std::runtime_error(fmt::format("Face count mismatch: expected {}, got {}",
                                                 m_Data.surfaces.size(), faceCount));
        }

        file.readUInt32(); // Lightmap count
        glm::ivec2 lightmapBlockSize;
        lightmapBlockSize.x = file.readInt32(); // Lightmap texture wide
        lightmapBlockSize.y = file.readInt32(); // Lightmap texture tall
        m_pLoadingState->customLightmapTexSize = lightmapBlockSize;
        LightmapFileFormat::Format format = (LightmapFileFormat::Format)file.readByte();

        if (format != LightmapFileFormat::Format::RGBF32) {
            throw std::runtime_error("Unsupported lightmap format, expected RGBF32");
        }

        // Lightmap texture
        m_pLoadingState->customLightmapTex.resize((size_t)lightmapBlockSize.x *
                                                  lightmapBlockSize.y);
        file.readBytes(reinterpret_cast<uint8_t *>(m_pLoadingState->customLightmapTex.data()),
                       m_pLoadingState->customLightmapTex.size() * sizeof(glm::vec3));

        auto &patchBuf = m_pLoadingState->patchBuffer;
        patchBuf.clear();
        patchBuf.reserve(80000);

        // Face info
        for (uint32_t i = 0; i < faceCount; i++) {
            Surface &surf = m_Data.surfaces[i];

            uint32_t vertCount = file.readUInt32(); // Vertex count

            if (vertCount != (uint32_t)surf.m_iVertexCount) {
                throw std::runtime_error(
                    fmt::format("Face {}: Vertex count mismatch: expected {}, got {}", i,
                                surf.m_iVertexCount, vertCount));
            }

            glm::vec3 vI = file.readVec<glm::vec3>(); // vI
            glm::vec3 vJ = file.readVec<glm::vec3>(); // vJ
            glm::vec3 vPlaneOrigin = file.readVec<glm::vec3>(); // World position of (0, 0) plane coord.
            file.readVec<glm::vec2>(); // Offset of (0, 0) to get to plane coords
            file.readVec<glm::vec2>(); // Face size

            bool hasLightmap = file.readByte(); // Has lightmap

            if (hasLightmap) {
                glm::ivec2 lmSize = file.readVec<glm::ivec2>(); // Lightmap size
                surf.m_vCustomLMTexCoords.resize(surf.m_iVertexCount);

                // Lightmap tex coords
                for (uint32_t j = 0; j < vertCount; j++) {
                    glm::vec2 texCoords = file.readVec<glm::vec2>(); // Tex coord in luxels
                    surf.m_vCustomLMTexCoords[j] = texCoords / glm::vec2(lightmapBlockSize);
                }

                // Patches
                uint32_t patchCount = file.readUInt32(); // Patch count

                for (uint32_t j = 0; j < patchCount; j++) {
                    glm::vec2 coord = file.readVec<glm::vec2>();
                    float size = file.readFloat();
                    float k = size / 2.0f;

                    glm::vec3 org = vPlaneOrigin + coord.x * vI + coord.y * vJ;
                    glm::vec3 corners[4];

                    corners[0] = org - vI * k - vJ * k;
                    corners[1] = org + vI * k - vJ * k;
                    corners[2] = org + vI * k + vJ * k;
                    corners[3] = org - vI * k + vJ * k;

                    patchBuf.push_back(corners[0]);
                    patchBuf.push_back(corners[1]);

                    patchBuf.push_back(corners[1]);
                    patchBuf.push_back(corners[2]);

                    patchBuf.push_back(corners[2]);
                    patchBuf.push_back(corners[3]);

                    patchBuf.push_back(corners[3]);
                    patchBuf.push_back(corners[0]);
                }
            }
        }

        patchBuf.shrink_to_fit();
        printi("Custom lightmaps: Patches use {:.1f} MiB of VRAM",
               patchBuf.size() * sizeof(patchBuf[0]) / 1024.0 / 1024.0);
    } catch (const std::exception &e) {
        printw("Custom lightmaps: failed to load: {}", e.what());
        m_pLoadingState->customLightmapTex.clear();
    }
}

void SceneRenderer::finishLoadCustomLightmaps() {
    if (m_pLoadingState->customLightmapTex.empty()) {
        return;
    }

    auto &blockSize = m_pLoadingState->customLightmapTexSize;
    auto &blockData = m_pLoadingState->customLightmapTex;

    // Load the block into the GPU
    Texture2D &texture = m_Data.customLightmapBlockTex;
    TextureFilter filter =
        r_filter_lm.getValue() ? TextureFilter::Bilinear : TextureFilter::Nearest;
    texture.create("SceneRenderer: custom lightmap block");
    texture.setWrapMode(TextureWrapMode::Clamp);
    texture.setFilter(filter);
    texture.initTexture(GraphicsFormat::RGB16F, blockSize.x, blockSize.y, false, GL_RGB, GL_FLOAT,
                        blockData.data());

    blockData.clear();

    // Load patches into the GPU
    auto &patchBuf = m_pLoadingState->patchBuffer;
    if (!patchBuf.empty()) {
        m_Data.patchesVerts = (uint32_t)patchBuf.size();
        m_Data.patchesVao.create();
        m_Data.patchesVbo.create(GL_ARRAY_BUFFER , "SceneRenderer: Patches");

        glBindVertexArray(m_Data.patchesVao);
        m_Data.patchesVbo.bind();
        m_Data.patchesVbo.init(m_Data.patchesVerts * sizeof(glm::vec3), patchBuf.data(),
                               GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    }

    m_Data.bCustomLMLoaded = true;
    printi("Custom lightmaps: loaded");
}

void SceneRenderer::asyncCreateSurfaceObjects() {
    printi("Surface objects: creating");

    std::vector<SurfaceVertex> vertices;
    m_pLoadingState->allVertices.reserve(bsp::MAX_MAP_VERTS);
    unsigned maxEboSize = 0; // Sum of vertex counts of all surfaces + surface count (to account for primitive restart element)

    m_pLoadingState->surfVertices.resize(m_Data.surfaces.size());
    m_pLoadingState->bspLMCoordsBuf.reserve(bsp::MAX_MAP_VERTS);
    m_pLoadingState->customLMCoordsBuf.reserve(bsp::MAX_MAP_VERTS);

    for (size_t i = 0; i < m_Data.surfaces.size(); i++) {
        Surface &surf = m_Data.surfaces[i];
        const SurfaceRenderer::Surface &baseSurf = m_Surf.getSurface(i);

        vertices.clear();
        vertices.reserve(surf.m_iVertexCount);
        maxEboSize += surf.m_iVertexCount + 1; //!< +1 to account for primitive reset
        surf.m_nFirstVertex = (uint16_t)m_pLoadingState->allVertices.size();

        glm::vec3 normal = baseSurf.pPlane->vNormal;
        if (baseSurf.iFlags & SURF_PLANEBACK) {
            normal = -normal;
        }

        const bsp::BSPTextureInfo &texInfo = *baseSurf.pTexInfo;
        const Material &material = *surf.m_pMat;

        for (size_t j = 0; j < baseSurf.vVertices.size(); j++) {
            SurfaceVertex v;

            // Position
            v.position = baseSurf.vVertices[j];

            // Normal
            v.normal = normal;

            // Texture
            v.texture.s = glm::dot(v.position, texInfo.vS) + texInfo.fSShift;
            v.texture.s /= material.getWide();

            v.texture.t = glm::dot(v.position, texInfo.vT) + texInfo.fTShift;
            v.texture.t /= material.getTall();

            // BSP lightmap texture
            glm::vec2 bspLmCoord = glm::vec2(0, 0);
            if (surf.m_BSPLMSize.x != 0) {
                bspLmCoord.s = glm::dot(v.position, texInfo.vS);
                bspLmCoord.t = glm::dot(v.position, texInfo.vT);
                bspLmCoord += glm::vec2(texInfo.fSShift, texInfo.fTShift);
                bspLmCoord -= surf.m_vTextureMins;
                bspLmCoord += glm::vec2(BSP_LIGHTMAP_DIVISOR / 2); // Shift by half-texel
                bspLmCoord += surf.m_BSPLMOffset * BSP_LIGHTMAP_DIVISOR;
                bspLmCoord /= BSP_LIGHTMAP_BLOCK_SIZE * BSP_LIGHTMAP_DIVISOR;
            }
            m_pLoadingState->bspLMCoordsBuf.push_back(bspLmCoord);

            // Custom lightmap texture
            glm::vec2 customLmCoord = glm::vec2(0, 0);
            if (m_Data.bCustomLMLoaded && !surf.m_vCustomLMTexCoords.empty()) {
                customLmCoord = surf.m_vCustomLMTexCoords[j];
            }
            m_pLoadingState->customLMCoordsBuf.push_back(customLmCoord);

            vertices.push_back(v);

            if (m_pLoadingState->allVertices.size() >= std::numeric_limits<uint16_t>::max()) {
                printe("Surface objects: vertex limit reached.");
                throw std::runtime_error("vertex limit reached");
            }

            m_pLoadingState->allVertices.push_back(v);
        }

        auto fnGetRandColor = []() { return (rand() % 256) / 255.f; };
        surf.m_Color = {fnGetRandColor(), fnGetRandColor(), fnGetRandColor()};

        m_pLoadingState->surfVertices[i] = std::move(vertices);
    }

    m_pLoadingState->maxEboSize = maxEboSize;
}

void SceneRenderer::finishCreateSurfaceObjects() {
    unsigned maxEboSize = m_pLoadingState->maxEboSize;
    auto &allVertices = m_pLoadingState->allVertices;

    // Lightmap coord buffers
    m_Data.bspLightmapCoords.create(GL_ARRAY_BUFFER, "BSP lm coords");
    m_Data.bspLightmapCoords.bind();
    m_Data.bspLightmapCoords.init(sizeof(glm::vec2) * m_pLoadingState->bspLMCoordsBuf.size(),
                                  m_pLoadingState->bspLMCoordsBuf.data(), GL_STATIC_DRAW);

    m_Data.customLightmapCoords.create(GL_ARRAY_BUFFER, "Custom lm coords");
    m_Data.customLightmapCoords.bind();
    m_Data.customLightmapCoords.init(sizeof(glm::vec2) * m_pLoadingState->customLMCoordsBuf.size(),
                                     m_pLoadingState->customLMCoordsBuf.data(), GL_STATIC_DRAW);

    // Surface vertices
    m_Data.surfVbo.create(GL_ARRAY_BUFFER, "SceneRenderer: Surface vertices");
    m_Data.surfVbo.bind();
    m_Data.surfVbo.init(sizeof(SurfaceVertex) * allVertices.size(), allVertices.data(),
                        GL_STATIC_DRAW);
    m_Data.surfVbo.unbind();

    // Set lightmap type
    if (m_Data.customLightmapBlockTex.getId()) {
        m_Data.lightmapType = LightmapType::Custom;
    } else {
        m_Data.lightmapType = LightmapType::BSP;
    }

    // Tinting colors
#ifdef RENDERER_SUPPORT_TINTING
    {
        std::vector<glm::vec4> defaultTint(allVertices.size(), glm::vec4(0, 0, 0, 0));
        m_Data.surfTintBuf.create(GL_ARRAY_BUFFER, "Surface tinting");
        m_Data.surfTintBuf.init(sizeof(glm::vec4) * defaultTint.size(), defaultTint.data(),
                                GL_DYNAMIC_DRAW);
    }
#endif

    // Update the VAO
    updateVao();

    // Create EBO
    printi("Maximum EBO size: {} B", maxEboSize * sizeof(uint16_t));
    printi("Vertex count: {}", allVertices.size());
    m_Data.surfEboData.resize(maxEboSize);
    m_Data.surfEbo.create(GL_ELEMENT_ARRAY_BUFFER, "SceneRenderer: Surface vertex indices");
    m_Data.surfEbo.bind();
    m_Data.surfEbo.init(maxEboSize * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);
    m_Data.surfEbo.unbind();

    printi("Surface objects: finished");
}

void SceneRenderer::updateVao() {
    static_assert(sizeof(SurfaceVertex) == sizeof(float) * 8, "Size of Vertex is invalid");

    m_Data.surfVao.create();
    glBindVertexArray(m_Data.surfVao);

    // Common attributes
    m_Data.surfVbo.bind();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(SurfaceVertex),
                          reinterpret_cast<void *>(offsetof(SurfaceVertex, position)));
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(SurfaceVertex),
                          reinterpret_cast<void *>(offsetof(SurfaceVertex, normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(SurfaceVertex),
                          reinterpret_cast<void *>(offsetof(SurfaceVertex, texture)));

    // Lightmap attributes
    if (m_Data.lightmapType == LightmapType::BSP) {
        m_Data.bspLightmapCoords.bind();
    } else {
        m_Data.customLightmapCoords.bind();
    }

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(glm::vec2), reinterpret_cast<void *>(0));

#ifdef RENDERER_SUPPORT_TINTING
    // Tinting
    m_Data.surfTintBuf.bind();
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, false, sizeof(glm::vec4), reinterpret_cast<void *>(0));
#endif

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SceneRenderer::loadTextures() {
    size_t surfCount = m_Surf.getSurfaceCount();
    for (size_t i = 0; i < surfCount; i++) {
        const SurfaceRenderer::Surface &surf = m_Surf.getSurface(i);
        const bsp::BSPMipTex &tex = m_pLevel->getTextures().at(surf.pTexInfo->iMiptex);
        Material *mat = m_pEngine->getMaterial(tex);

        if (!mat) {
            mat = MaterialSystem::get().getNullMaterial();
        }

        m_Surf.setSurfaceMaterial(i, mat);
    }

    m_Surf.refreshMaterialIndexes();
}

void SceneRenderer::loadSkyBox() {
    auto texture = std::make_unique<TextureCube>();
    texture->create("Skybox");

    // Filename suffix for side
    constexpr const char *suffixes[] = {"rt", "lf", "up", "dn", "bk", "ft"};

    // Get sky name
    const bsp::Level::EntityListItem &worldspawn = m_pLevel->getEntities().getWorldspawn();
    std::string skyname = worldspawn.getValue<std::string>("skyname", "desert");

    // Load images
    int loadedSidesCount = 0;
    std::vector<uint8_t> sidesData[6];
    int cubeWide = 0, cubeTall = 0;

    for (int i = 0; i < 6; i++) {
        // Try TGA first
        fs::path path = getFileSystem().findExistingFile(
            "assets:gfx/env/" + skyname + suffixes[i] + ".tga", std::nothrow);

        if (path.empty()) {
            // Try BMP instead
            path = getFileSystem().findExistingFile(
                "assets:gfx/env/" + skyname + suffixes[i] + ".bmp", std::nothrow);
        }

        if (path.empty()) {
            printe("Sky {}{} not found", skyname, suffixes[i]);
            continue;
        }

        int width, height, channelNum;
        uint8_t *data = stbi_load(path.u8string().c_str(), &width, &height, &channelNum, 3);

        if (!data) {
            printe("Failed to load {}: ", path.u8string(), stbi_failure_reason());
            continue;
        }

        // Validate size
        if (width != height) {
            printe("{}: image is not square", path.u8string());
            continue;
        } else if (cubeWide == 0 || cubeTall == 0) {
            // Set the cubemap size
            cubeWide = width;
            cubeTall = height;
        } else if (cubeWide != width) {
            printe("{}: image size is different ({} vs {})", path.u8string(), width,
                    cubeWide);
            continue;
        }

        // Save the data
        if (i == 2) {
            // Top sky texture needs to be rotated CCW
            sidesData[i] = rotateImage90CCW(data, width, height);
        } else if (i == 3) {
            // Bottom sky texture needs to be rotated CW
            sidesData[i] = rotateImage90CW(data, width, height);
        } else {
            sidesData[i].resize(width * height * 3);
            std::copy(data, data + sidesData[i].size(), sidesData[i].begin());
        }

        stbi_image_free(data);
        loadedSidesCount++;
    }

    if (loadedSidesCount == 6) {
        // Load the cubemap
        const void *datas[6];

        for (int i = 0; i < 6; i++) {
            datas[i] = sidesData[i].data();
        }

        texture->initTexture(GraphicsFormat::RGB8, cubeWide, cubeTall, true, GL_RGB,
                             GL_UNSIGNED_BYTE, datas);
    } else {
        // Load error cubemap
        int size = CheckerboardImage::get().size;
        const void *data = CheckerboardImage::get().data.data();
        const void *datas[6];
        std::fill(datas, datas + 6, data);

        texture->initTexture(GraphicsFormat::RGB8, size, size, true, GL_RGB, GL_UNSIGNED_BYTE,
                             datas);
    }

    texture->setWrapMode(TextureWrapMode::Clamp);
    m_pSkyboxMaterial->setTexture(0, std::move(texture));
}

void SceneRenderer::finishLoading() {
    m_LoadingStatus = LoadingStatus::NotLoading;
    m_pLoadingState = nullptr;
}

std::vector<uint8_t> SceneRenderer::rotateImage90CW(uint8_t *data, int wide, int tall) {
    std::vector<uint8_t> newData(3 * wide * tall);

    auto fnGetPixelPtr = [](uint8_t *data, int wide, [[maybe_unused]] int tall, int col, int row) {
        return data + row * wide * 3 + col * 3;
    };

    for (int col = 0; col < wide; col++) {
        for (int row = 0; row < tall; row++) {
            uint8_t *pold = fnGetPixelPtr(data, wide, tall, col, tall - row - 1);
            uint8_t *pnew = fnGetPixelPtr(newData.data(), tall, wide, row, col);
            memcpy(pnew, pold, 3);
        }
    }

    return newData;
}

std::vector<uint8_t> SceneRenderer::rotateImage90CCW(uint8_t *data, int wide, int tall) {
    std::vector<uint8_t> newData(3 * wide * tall);

    auto fnGetPixelPtr = [](uint8_t *data, int wide, [[maybe_unused]] int tall, int col, int row) {
        return data + row * wide * 3 + col * 3;
    };

    for (int col = 0; col < wide; col++) {
        for (int row = 0; row < tall; row++) {
            uint8_t *pold = fnGetPixelPtr(data, wide, tall, wide - col - 1, row);
            uint8_t *pnew = fnGetPixelPtr(newData.data(), tall, wide, row, col);
            memcpy(pnew, pold, 3);
        }
    }

    return newData;
}

void SceneRenderer::frameSetup(float flSimTime, float flTimeDelta) {
    appfw::Prof prof("Frame Setup");

    prepareHdrFramebuffer();
    setupViewContext();

    // Depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Backface culling
    if (m_Data.viewContext.getCulling() != SurfaceRenderer::Cull::None) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW);
        glCullFace(m_Data.viewContext.getCulling() == SurfaceRenderer::Cull::Back ? GL_BACK : GL_FRONT);
    }

    // Seamless cubemap filtering
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Fill global uniform object
    m_GlobalUniform.mMainProj = m_Data.viewContext.getProjectionMatrix();
    m_GlobalUniform.mMainView = m_Data.viewContext.getViewMatrix();
    m_GlobalUniform.vMainViewOrigin = glm::vec4(m_Data.viewContext.getViewOrigin(), 0);
    m_GlobalUniform.vflParams1.x = r_texgamma.getValue();
    m_GlobalUniform.vflParams1.y = r_gamma.getValue();
    m_GlobalUniform.vflParams1.z = flSimTime;
    m_GlobalUniform.vflParams1.w = flTimeDelta;
    m_GlobalUniform.viParams1.x = r_texture.getValue();
    m_GlobalUniform.viParams1.y = r_shading.getValue();

    if (m_Data.lightmapType == LightmapType::BSP) {
        m_GlobalUniform.vflParams2.x = 1.5f;
    } else {
        m_GlobalUniform.vflParams2.x = 1.0f;
    }
    
    m_GlobalUniformBuffer.bind();
    m_GlobalUniformBuffer.update(0, sizeof(m_GlobalUniform), &m_GlobalUniform);
    glBindBufferBase(GL_UNIFORM_BUFFER, GLOBAL_UNIFORM_BIND, m_GlobalUniformBuffer.getId());

    // Bind lightmap - will be used for world and brush ents
    bindLightmapBlock();
}

void SceneRenderer::frameEnd() {
    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void SceneRenderer::prepareHdrFramebuffer() {
    if (m_bNeedRefreshFB) {
        recreateFramebuffer();
        m_bNeedRefreshFB = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_nHdrFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, m_vViewportSize.x, m_vViewportSize.y);
}

void SceneRenderer::setupViewContext() {
    if (r_cull.getValue() <= 0) {
        m_Data.viewContext.setCulling(SurfaceRenderer::Cull::None);
    } else if (r_cull.getValue() == 1) {
        m_Data.viewContext.setCulling(SurfaceRenderer::Cull::Back);
    } else if (r_cull.getValue() >= 2) {
        m_Data.viewContext.setCulling(SurfaceRenderer::Cull::Front);
    }
}

void SceneRenderer::bindLightmapBlock() {
    // Bind lightmap block texture
    if (m_Data.lightmapType == LightmapType::BSP) {
        if (m_Data.bspLightmapBlockTex.getId() != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_Data.bspLightmapBlockTex.getId());
        }
    } else {
        if (m_Data.customLightmapBlockTex.getId() != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_Data.customLightmapBlockTex.getId());
        }
    }
}

void SceneRenderer::drawWorldSurfaces() {
    // Prepare list of visible world surfaces
    {
        appfw::Prof prof("BSP traverse");
        m_Surf.setContext(&m_Data.viewContext);
        m_Surf.calcWorldSurfaces();
    }

    // Draw visible surfaces
    appfw::Prof prof("Draw world");

    if (r_ebo.getValue()) {
        drawWorldSurfacesIndexed();
    } else {
        drawWorldSurfacesVao();
    }
}

void SceneRenderer::drawWorldSurfacesVao() {
    auto &textureChain = m_Data.viewContext.getWorldTextureChain();
    auto &textureChainFrames = m_Data.viewContext.getWorldTextureChainFrames();
    unsigned frame = m_Data.viewContext.getWorldTextureChainFrame();
    unsigned drawnSurfs = 0;

    glBindVertexArray(m_Data.surfVao);

    for (size_t i = 0; i < textureChain.size(); i++) {
        if (textureChainFrames[i] != frame) {
            continue;
        }

        // Bind material
        const Material &mat = *m_Data.surfaces[textureChain[i][0]].m_pMat;
        mat.activateTextures();
        mat.enableShader(SHADER_TYPE_WORLD_IDX);

        for (unsigned j : textureChain[i]) {
            Surface &surf = m_Data.surfaces[j];
            glDrawArrays(GL_TRIANGLE_FAN, surf.m_nFirstVertex, (GLsizei)surf.m_iVertexCount);
        }

        drawnSurfs += (unsigned)textureChain[i].size();
    }

    glBindVertexArray(0);

    m_Stats.uRenderedWorldPolys = drawnSurfs;
    m_Stats.uDrawCallCount += drawnSurfs;
}

void SceneRenderer::drawWorldSurfacesIndexed() {
    AFW_ASSERT(!m_Data.surfEboData.empty());

    auto &textureChain = m_Data.viewContext.getWorldTextureChain();
    auto &textureChainFrames = m_Data.viewContext.getWorldTextureChainFrames();
    unsigned frame = m_Data.viewContext.getWorldTextureChainFrame();
    unsigned drawnSurfs = 0;

    // Bind buffers
    glBindVertexArray(m_Data.surfVao);
    m_Data.surfEbo.bind();
    glPrimitiveRestartIndex(PRIMITIVE_RESTART_IDX);
    glEnable(GL_PRIMITIVE_RESTART);

    for (size_t i = 0; i < textureChain.size(); i++) {
        if (textureChainFrames[i] != frame) {
            continue;
        }

        // Bind material
        const Material &mat = *m_Data.surfaces[textureChain[i][0]].m_pMat;
        mat.activateTextures();
        mat.enableShader(SHADER_TYPE_WORLD_IDX);

        // Fill the EBO
        unsigned eboIdx = 0;

        for (unsigned j : textureChain[i]) {
            Surface &surf = m_Data.surfaces[j];
            uint16_t vertCount = (uint16_t)surf.m_iVertexCount;

            for (uint16_t k = 0; k < vertCount; k++) {
                m_Data.surfEboData[eboIdx + k] = surf.m_nFirstVertex + k;
            }

            eboIdx += vertCount;
            m_Data.surfEboData[eboIdx] = PRIMITIVE_RESTART_IDX;
            eboIdx++;
        }

        // Decrement EBO size to remove last PRIMITIVE_RESTART_IDX
        eboIdx--;

        // Update EBO
        m_Data.surfEbo.update(0, eboIdx * sizeof(uint16_t), m_Data.surfEboData.data());

        // Draw elements
        glDrawElements(GL_TRIANGLE_FAN, eboIdx, GL_UNSIGNED_SHORT, nullptr);

        drawnSurfs += (unsigned)textureChain[i].size();
        m_Stats.uDrawCallCount++;
    }

    if (r_wireframe.getValue()) {
        unsigned eboIdx = 0;

        // Fill EBO with all visible surfaces
        for (size_t i = 0; i < textureChain.size(); i++) {
            if (textureChainFrames[i] != frame) {
                continue;
            }

            for (unsigned j : textureChain[i]) {
                Surface &surf = m_Data.surfaces[j];
                uint16_t vertCount = (uint16_t)surf.m_iVertexCount;

                for (uint16_t k = 0; k < vertCount; k++) {
                    m_Data.surfEboData[eboIdx + k] = surf.m_nFirstVertex + k;
                }

                eboIdx += vertCount;
                m_Data.surfEboData[eboIdx] = PRIMITIVE_RESTART_IDX;
                eboIdx++;
            }
        }

        if (eboIdx > 0) {
            // Decrement EBO size to remove last PRIMITIVE_RESTART_IDX
            eboIdx--;

            ShaderInstance *shaderInstance =
                Shaders::brushent.getShaderInstance(SHADER_TYPE_BRUSH_MODEL);
            auto &shaderInfo = shaderInstance->getShader<BrushEntityShader>();
            shaderInstance->enable();
            shaderInfo.m_uRenderMode.set(kRenderTransColor);
            shaderInfo.m_uFxAmount.set(1.0);
            shaderInfo.m_uFxColor.set({1.0, 1.0, 1.0});

            // Update EBO
            m_Data.surfEbo.update(0, eboIdx * sizeof(uint16_t), m_Data.surfEboData.data());

            // Draw elements
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0f, -1.f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLE_FAN, eboIdx, GL_UNSIGNED_SHORT, nullptr);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_POLYGON_OFFSET_LINE);
            m_Stats.uDrawCallCount++;
        }
    }

    glDisable(GL_PRIMITIVE_RESTART);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    m_Stats.uRenderedWorldPolys = drawnSurfs;
}

void SceneRenderer::drawSkySurfaces() {
    appfw::Prof prof("Draw sky");

    if (!m_Data.viewContext.getSkySurfaces().empty()) {
        glDepthFunc(GL_LEQUAL);

        m_pSkyboxMaterial->activateTextures();
        m_pSkyboxMaterial->enableShader(SHADER_TYPE_WORLD_IDX);

        if (r_ebo.getValue()) {
            drawSkySurfacesIndexed();
        } else {
            drawSkySurfacesVao();
        }

        glDepthFunc(GL_LESS);
    }
}

void SceneRenderer::drawSkySurfacesVao() {
    glBindVertexArray(m_Data.surfVao);

    for (unsigned i : m_Data.viewContext.getSkySurfaces()) {
        Surface &surf = m_Data.surfaces[i];
        glDrawArrays(GL_TRIANGLE_FAN, surf.m_nFirstVertex, (GLsizei)surf.m_iVertexCount);
    }

    glBindVertexArray(0);
    m_Stats.uRenderedSkyPolys = (unsigned)m_Data.viewContext.getSkySurfaces().size();
    m_Stats.uDrawCallCount += m_Stats.uRenderedSkyPolys;}

void SceneRenderer::drawSkySurfacesIndexed() {
    AFW_ASSERT(!m_Data.surfEboData.empty());

    // Bind buffers
    glBindVertexArray(m_Data.surfVao);
    m_Data.surfEbo.bind();
    glPrimitiveRestartIndex(PRIMITIVE_RESTART_IDX);
    glEnable(GL_PRIMITIVE_RESTART);

    // Fill the EBO
    unsigned eboIdx = 0;

    for (unsigned j : m_Data.viewContext.getSkySurfaces()) {
        Surface &surf = m_Data.surfaces[j];
        uint16_t vertCount = (uint16_t)surf.m_iVertexCount;

        for (uint16_t k = 0; k < vertCount; k++) {
            m_Data.surfEboData[eboIdx + k] = surf.m_nFirstVertex + k;
        }

        eboIdx += vertCount;
        m_Data.surfEboData[eboIdx] = PRIMITIVE_RESTART_IDX;
        eboIdx++;
    }

    // Decrement EBO size to remove last PRIMITIVE_RESTART_IDX
    eboIdx--;

    // Update EBO
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboIdx * sizeof(uint16_t), m_Data.surfEboData.data());

    // Draw elements
    glDrawElements(GL_TRIANGLE_FAN, eboIdx, GL_UNSIGNED_SHORT, nullptr);

    m_Stats.uRenderedSkyPolys = (unsigned)m_Data.viewContext.getSkySurfaces().size();
    m_Stats.uDrawCallCount++;

    glDisable(GL_PRIMITIVE_RESTART);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SceneRenderer::drawSolidEntities() {
    appfw::Prof prof("Solid");

    // Sort opaque entities
    auto sortFn = [this](ClientEntity *const &lhs, ClientEntity *const &rhs) {
        // Sort by model type
        int lhsmodel = (int)lhs->getModel()->getType();
        int rhsmodel = (int)rhs->getModel()->getType();

        if (lhsmodel > rhsmodel) {
            return false;
        } else if (lhsmodel < rhsmodel) {
            return true;
        }

        // Sort by render mode
        return lhs->getRenderModeRank() < lhs->getRenderModeRank();
    };

    std::sort(m_SolidEntityList.begin(), m_SolidEntityList.end(), sortFn);

    for (ClientEntity *pClent : m_SolidEntityList) {
        switch (pClent->getModel()->getType()) {
        case ModelType::Brush: {
            drawSolidBrushEntity(pClent);
            break;
        }
        }
    }

    setRenderMode(kRenderNormal);
}

void SceneRenderer::drawSolidTriangles() {
    appfw::Prof prof("Solid Triangles");
    m_pEngine->drawNormalTriangles(m_Stats.uDrawCallCount);
}

void SceneRenderer::drawTransEntities() {
    appfw::Prof prof("Trans");

    if (!r_nosort.getValue()) {
        // Sort entities based on render mode and distance
        auto sortFn = [this](ClientEntity *const &lhs, ClientEntity *const &rhs) {
            // Sort by render mode
            int lhsRank = lhs->getRenderModeRank();
            int rhsRank = lhs->getRenderModeRank();

            if (lhsRank > rhsRank) {
                return false;
            } else if (lhsRank < rhsRank) {
                return true;
            }

            // Sort by distance
            auto fnGetOrigin = [](const ClientEntity *ent) {
                Model *model = ent->getModel();
                if (model->getType() == ModelType::Brush) {
                    glm::vec3 avg = (model->getMins() + model->getMaxs()) * 0.5f;
                    return ent->getOrigin() + avg;
                } else {
                    return ent->getOrigin();
                }
            };

            glm::vec3 org1 = fnGetOrigin(lhs) - m_Data.viewContext.getViewOrigin();
            glm::vec3 org2 = fnGetOrigin(rhs) - m_Data.viewContext.getViewOrigin();
            float dist1 = glm::length2(org1);
            float dist2 = glm::length2(org2);

            return dist1 > dist2;
        };

        std::sort(m_TransEntityList.begin(), m_TransEntityList.end(), sortFn);
    }

    for (ClientEntity *pClent : m_TransEntityList) {
        switch (pClent->getModel()->getType()) {
        case ModelType::Brush: {
            drawBrushEntity(pClent);
            break;
        }
        }
    }

    setRenderMode(kRenderNormal);
}

void SceneRenderer::drawTransTriangles() {
    appfw::Prof prof("Trans Triangles");
    m_pEngine->drawTransTriangles(m_Stats.uDrawCallCount);
}

void SceneRenderer::drawSolidBrushEntity(ClientEntity *clent) {
    Model *model = clent->getModel();

    ShaderInstance *shaderInstance =
        Shaders::brushent.getShaderInstance(SHADER_TYPE_BRUSH_MODEL_IDX);
    auto &shader = shaderInstance->getShader<BrushEntityShader>();
    shaderInstance->enable();

    // Frustum culling
    glm::vec3 mins, maxs;

    if (isVectorNull(clent->getAngles())) {
        mins = clent->getOrigin() + model->getMins();
        maxs = clent->getOrigin() + model->getMaxs();
    } else {
        glm::vec3 radius = glm::vec3(model->getRadius());
        mins = clent->getOrigin() - radius;
        maxs = clent->getOrigin() + radius;
    }

    if (m_Surf.cullBox(mins, maxs)) {
        return;
    }

    // Model transformation matrix
    glm::mat4 modelMat = glm::identity<glm::mat4>();
    modelMat = glm::rotate(modelMat, glm::radians(clent->getAngles().z), {1.0f, 0.0f, 0.0f});
    modelMat = glm::rotate(modelMat, glm::radians(clent->getAngles().x), {0.0f, 1.0f, 0.0f});
    modelMat = glm::rotate(modelMat, glm::radians(clent->getAngles().y), {0.0f, 0.0f, 1.0f});
    modelMat = glm::translate(modelMat, clent->getOrigin());
    shader.m_uModelMat.set(modelMat);

    // Render mode
    AFW_ASSERT(clent->getRenderMode() == kRenderNormal || clent->getRenderMode() == kRenderTransAlpha);
    setRenderMode(clent->getRenderMode());
    shader.m_uRenderMode.set(clent->getRenderMode());

    // Draw surfaces
    auto &surfs = m_Data.optBrushModels[model->getOptModelIdx()].surfs;
    Material *lastMat = nullptr;

    for (unsigned i : surfs) {
        Surface &surf = m_Data.surfaces[i];

        // TODO: cullSurface doesn't account for translations/rotations
        //if (m_Surf.cullSurface(m_Surf.getSurface(i))) {
        //    continue;
        //}

        if (r_texture.getValue() == 1) {
            // Set color
            Shaders::brushent.setColor(surf.m_Color);
        } else if (r_texture.getValue() == 2) {
            // Bind material
            if (lastMat != surf.m_pMat) {
                lastMat = surf.m_pMat;
                lastMat->activateTextures();
            }
        }

        glBindVertexArray(m_Data.surfVao);
        glDrawArrays(GL_TRIANGLE_FAN, surf.m_nFirstVertex, (GLsizei)surf.m_iVertexCount);
        m_Stats.uDrawCallCount++;
        m_Stats.uRenderedBrushEntPolys++;
    }
}

void SceneRenderer::drawBrushEntity(ClientEntity *clent) {
    Model *model = clent->getModel();

    ShaderInstance *shaderInstance =
        Shaders::brushent.getShaderInstance(SHADER_TYPE_BRUSH_MODEL_IDX);
    auto &shader = shaderInstance->getShader<BrushEntityShader>();
    shaderInstance->enable();

    // Frustum culling
    glm::vec3 mins, maxs;

    if (isVectorNull(clent->getAngles())) {
        mins = clent->getOrigin() + model->getMins();
        maxs = clent->getOrigin() + model->getMaxs();
    } else {
        glm::vec3 radius = glm::vec3(model->getRadius());
        mins = clent->getOrigin() - radius;
        maxs = clent->getOrigin() + radius;
    }

    if (m_Surf.cullBox(mins, maxs)) {
        return;
    }

    // Model transformation matrix
    glm::mat4 modelMat = glm::identity<glm::mat4>();
    modelMat = glm::rotate(modelMat, glm::radians(clent->getAngles().z), {1.0f, 0.0f, 0.0f});
    modelMat = glm::rotate(modelMat, glm::radians(clent->getAngles().x), {0.0f, 1.0f, 0.0f});
    modelMat = glm::rotate(modelMat, glm::radians(clent->getAngles().y), {0.0f, 0.0f, 1.0f});
    modelMat = glm::translate(modelMat, clent->getOrigin());
    shader.m_uModelMat.set(modelMat);

    // Render mode
    if (!r_notrans.getValue()) {
        setRenderMode(clent->getRenderMode());
        shader.m_uRenderMode.set(clent->getRenderMode());
        shader.m_uFxAmount.set(clent->getFxAmount() / 255.f);
        shader.m_uFxColor.set(glm::vec3(clent->getFxColor()) / 255.f);
    } else {
        setRenderMode(kRenderNormal);
        shader.m_uRenderMode.set(kRenderNormal);
    }

    bool needSort = false;

    switch (clent->getRenderMode()) {
    case kRenderTransTexture:
    case kRenderTransAdd:
        m_SortBuffer.clear();
        needSort = true;
        break;
    }

    // Draw surfaces of prepare them for sorting
    unsigned from = model->getFirstFace();
    unsigned to = from + model->getFaceNum();

    for (unsigned i = from; i < to; i++) {
        Surface &surf = m_Data.surfaces[i];

        // TODO: cullSurface doesn't account for translations/rotations
        //if (m_Surf.cullSurface(m_Surf.getSurface(i))) {
        //    continue;
        //}

        if (needSort && !r_nosort.getValue()) {
            // Add to sorting list
            m_SortBuffer.push_back(i);
        } else {
            // Draw now
            drawBrushEntitySurface(surf);
        }
    }

    // Sort and draw
    if (needSort && !r_nosort.getValue()) {
        auto sortFn = [this, clent](const size_t &lhsIdx, const size_t &rhsIdx) {
            glm::vec3 lhsOrg = m_Surf.getSurface(lhsIdx).vOrigin + clent->getOrigin();
            glm::vec3 rhsOrg = m_Surf.getSurface(rhsIdx).vOrigin + clent->getOrigin();
            float lhsDist = glm::dot(lhsOrg, m_Data.viewContext.getViewForward());
            float rhsDist = glm::dot(rhsOrg, m_Data.viewContext.getViewForward());

            return lhsDist > rhsDist;
        };

        std::sort(m_SortBuffer.begin(), m_SortBuffer.end(), sortFn);

        for (size_t i : m_SortBuffer) {
            Surface &surf = m_Data.surfaces[i];
            drawBrushEntitySurface(surf);
        }
    }
}

void SceneRenderer::drawBrushEntitySurface(Surface &surf) {
    // Bind material
    surf.m_pMat->activateTextures();
    glBindVertexArray(m_Data.surfVao);
    glDrawArrays(GL_TRIANGLE_FAN, surf.m_nFirstVertex, (GLsizei)surf.m_iVertexCount);
    m_Stats.uDrawCallCount++;
    m_Stats.uRenderedBrushEntPolys++;
}

void SceneRenderer::drawPatches() {
    appfw::Prof prof("Patches");

    ShaderInstance *shaderInstance =
        Shaders::patches.getShaderInstance(SHADER_TYPE_CUSTOM_IDX);
    auto &shader = shaderInstance->getShader<PatchesShader>();
    shaderInstance->enable();
    shader.setupSceneUniforms(*this);

    glBindVertexArray(m_Data.patchesVao);
    glPointSize(5);
    glDrawArrays(GL_LINES, 0, m_Data.patchesVerts);
    m_Stats.uDrawCallCount++;
    glBindVertexArray(0);
}

void SceneRenderer::doPostProcessing() {
    appfw::Prof prof("Post-Processing");

    ShaderInstance *shaderInstance = Shaders::postprocess.getShaderInstance(SHADER_TYPE_BLIT_IDX);
    shaderInstance->enable();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_nColorBuffer.getId());

    glBindVertexArray(m_nQuadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void SceneRenderer::setRenderMode(RenderMode mode) {
    switch (mode) {
    case kRenderNormal:
    default:
        glDisable(GL_BLEND);
        break;
    case kRenderTransColor:
    case kRenderTransTexture:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case kRenderTransAlpha:
        glDisable(GL_BLEND);
        break;
    case kRenderGlow:
    case kRenderTransAdd:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    }
}
