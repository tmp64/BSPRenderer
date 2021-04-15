#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <renderer/stb_image.h>
#include <renderer/scene_renderer.h>
#include <imgui.h>
#include <gui_app/imgui_controls.h>

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
ConVar<float> r_gamma("r_gamma", 2.2f, "Gamma");
ConVar<float> r_texgamma("r_texgamma", 2.2f, "Texture gamma");
ConVar<int> r_texture("r_texture", 2,
                      "Which texture should be used:\n  0 - white color, 1 - random color, 2 - texture");
ConVar<int> r_lighting("r_lighting", 2,
                       "Lighting method:\n  0 - fullbright, 1 - shaded, 2 - BSP lightmaps, 3 - custom lightmaps");
ConVar<int> r_tonemap("r_tonemap", 2, "HDR tonemapping method:\n  0 - none (clamp to 1), 1 - Reinhard, 2 - exposure");
ConVar<float> r_exposure("r_exposure", 0.5f, "Picture exposure");
ConVar<float> r_skybright("r_skybright", 3.f, "Sky brightness");
ConVar<float> r_skybright_ldr("r_skybright", 1.f, "Sky brightness for LDR (BSP lightmpas)");
ConVar<bool> r_ebo("r_ebo", true, "Use indexed rendering");

//----------------------------------------------------------------
// WorldShader
//----------------------------------------------------------------
SceneRenderer::WorldShader::WorldShader()
    : BaseShader("SceneRenderer_WorldShader")
    , m_ViewMat(this, "uViewMatrix")
    , m_ProjMat(this, "uProjMatrix")
    , m_Color(this, "uColor")
    , m_LightingType(this, "uLightingType")
    , m_TextureType(this, "uTextureType")
    , m_Texture(this, "uTexture")
    , m_LMTexture(this, "uLMTexture")
    , m_TexGamma(this, "uTexGamma") {}

void SceneRenderer::WorldShader::create() {
    createProgram();
    createVertexShader("shaders/scene/world.vert", "assets");
    createFragmentShader("shaders/scene/world.frag", "assets");
    linkProgram();
}

void SceneRenderer::WorldShader::setupUniforms(SceneRenderer &scene) {
    m_ProjMat.set(scene.m_Data.viewContext.getProjectionMatrix());
    m_ViewMat.set(scene.m_Data.viewContext.getViewMatrix());
    m_LightingType.set(r_lighting.getValue());
    m_TextureType.set(r_texture.getValue());
    m_Texture.set(0);
    m_LMTexture.set(1);
    m_TexGamma.set(r_texgamma.getValue());
}

void SceneRenderer::WorldShader::setColor(const glm::vec3 &c) { m_Color.set(c); }

//----------------------------------------------------------------
// SkyBoxShader
//----------------------------------------------------------------
SceneRenderer::SkyBoxShader::SkyBoxShader()
    : BaseShader("SceneRenderer_WorldShader")
    , m_ViewMat(this, "uViewMatrix")
    , m_ProjMat(this, "uProjMatrix")
    , m_Texture(this, "uTexture")
    , m_TexGamma(this, "uTexGamma")
    , m_Brightness(this, "uBrightness")
    , m_ViewOrigin(this, "uViewOrigin") {}

void SceneRenderer::SkyBoxShader::create() {
    createProgram();
    createVertexShader("shaders/scene/skybox.vert", "assets");
    createFragmentShader("shaders/scene/skybox.frag", "assets");
    linkProgram();
}

void SceneRenderer::SkyBoxShader::setupUniforms(SceneRenderer &scene) {
    m_ProjMat.set(scene.m_Data.viewContext.getProjectionMatrix());
    m_ViewMat.set(scene.m_Data.viewContext.getViewMatrix());
    m_Texture.set(0);
    m_TexGamma.set(r_texgamma.getValue());
    m_ViewOrigin.set(scene.m_Data.viewContext.getViewOrigin());

    if (r_lighting.getValue() == 2) {
        m_Brightness.set(r_skybright_ldr.getValue());
    } else {
        m_Brightness.set(r_skybright.getValue());
    }
}

//----------------------------------------------------------------
// PostProcessShader
//----------------------------------------------------------------
SceneRenderer::PostProcessShader::PostProcessShader()
    : BaseShader("SceneRenderer_PostProcessShader")
    , m_Tonemap(this, "uTonemap")
    , m_Exposure(this, "uExposure")
    , m_Gamma(this, "uGamma") {}

void SceneRenderer::PostProcessShader::create() {
    createProgram();
    createVertexShader("shaders/scene/post_processing.vert", "assets");
    createFragmentShader("shaders/scene/post_processing.frag", "assets");
    linkProgram();
}

void SceneRenderer::PostProcessShader::setupUniforms() {
    if (r_lighting.getValue() == 2) {
        // No tonemapping for BSP lightmaps
        m_Tonemap.set(0);
    } else {
        m_Tonemap.set(r_tonemap.getValue());
    }
    m_Exposure.set(r_exposure.getValue());
    m_Gamma.set(r_gamma.getValue());
}

//----------------------------------------------------------------
// SceneRenderer
//----------------------------------------------------------------
SceneRenderer::SceneRenderer() {
    createScreenQuad();
}

SceneRenderer::~SceneRenderer() {}

void SceneRenderer::beginLoading(bsp::Level *level, const std::string &path, const char *tag) {
    AFW_ASSERT(level);

    if (m_pLevel == level) {
        return;
    }

    unloadLevel();

    m_Data.customLightmapPath = getFileSystem().findFileOrEmpty(path + ".lm", tag);

    m_pLevel = level;
    loadTextures();
    loadSkyBox();

    m_pLoadingState = std::make_unique<LoadingState>();
    m_LoadingStatus = LoadingStatus::CreateSurfaces;
    m_pLoadingState->createSurfacesResult = std::async(std::launch::async, [this]() { asyncCreateSurfaces(); });
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
            if (appfw::IsFutureReady(m_pLoadingState->createSurfacesResult)) {
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
                if (appfw::IsFutureReady(m_pLoadingState->loadBSPLightmapsResult)) {
                    m_pLoadingState->loadBSPLightmapsResult.get();
                    m_pLoadingState->loadBSPLightmapsResult = std::future<void>();
                    finishLoadBSPLightmaps();
                    m_pLoadingState->loadBSPLightmapsFinished = true;
                }
            }

            if (!m_pLoadingState->loadCustomLightmapsFinished) {
                if (appfw::IsFutureReady(m_pLoadingState->loadCustomLightmapsResult)) {
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
            if (appfw::IsFutureReady(m_pLoadingState->createSurfaceObjectsResult)) {
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

void SceneRenderer::renderScene(GLint targetFb) {
    AFW_ASSERT(m_pLevel);
    AFW_ASSERT(!isLoading());

    m_Stats.clear();
    appfw::Timer renderTimer;
    renderTimer.start();

    if (m_bNeedRefreshFB) {
        recreateFramebuffer();
        m_bNeedRefreshFB = false;
    }

    prepareHdrFramebuffer();
    setupViewContext();

    if (!m_Data.bCustomLMLoaded && r_lighting.getValue() == 3) {
        r_lighting.setValue(2);
        logWarn("Lighting type set to shaded since custom lightmaps are not loaded");
    }

    if (r_drawworld.getValue()) {
        drawWorldSurfaces();

        if (r_drawsky.getValue()) {
            drawSkySurfaces();
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, targetFb);
    doPostProcessing();

    renderTimer.stop();
    m_Stats.uTotalRenderingTime += (unsigned)renderTimer.elapsedMicroseconds();
}

void SceneRenderer::showDebugDialog(const char *title, bool *isVisible) {
    if (!isVisible || *isVisible) {
        if (!ImGui::Begin(title, isVisible)) {
            ImGui::End();
            return;
        }

        ImGui::Text("World: %u (%u + %u)", m_Stats.uRenderedWorldPolys + m_Stats.uRenderedSkyPolys,
                    m_Stats.uRenderedWorldPolys, m_Stats.uRenderedSkyPolys);
        ImGui::Text("Draw calls: %u", m_Stats.uDrawCallCount);
        ImGui::Separator();

        int timeLost = (int)m_Stats.uTotalRenderingTime.getSmoothed() - m_Stats.uWorldBSPTime.getSmoothed() -
                            m_Stats.uWorldRenderingTime.getSmoothed() - m_Stats.uSkyRenderingTime.getSmoothed() -
                            m_Stats.uPostProcessingTime.getSmoothed() - m_Stats.uSetupTime.getSmoothed();

        ImGui::Text("FPS: %.3f", 1000000.0 / m_Stats.uTotalRenderingTime.getSmoothed());
        ImGui::Text("Total: %2d.%03d ms", m_Stats.uTotalRenderingTime.getSmoothed() / 1000,
                    m_Stats.uTotalRenderingTime.getSmoothed() % 1000);
        ImGui::Text("Setup: %2d.%03d ms", m_Stats.uSetupTime.getSmoothed() / 1000,
                    m_Stats.uSetupTime.getSmoothed() % 1000);
        ImGui::Text("BSP: %2d.%03d ms", m_Stats.uWorldBSPTime.getSmoothed() / 1000,
                    m_Stats.uWorldBSPTime.getSmoothed() % 1000);
        ImGui::Text("World: %2d.%03d ms", m_Stats.uWorldRenderingTime.getSmoothed() / 1000,
                    m_Stats.uWorldRenderingTime.getSmoothed() % 1000);
        ImGui::Text("Sky: %2d.%03d ms", m_Stats.uSkyRenderingTime.getSmoothed() / 1000,
                    m_Stats.uSkyRenderingTime.getSmoothed() % 1000);
        ImGui::Text("Post: %2d.%03d ms", m_Stats.uPostProcessingTime.getSmoothed() / 1000,
                    m_Stats.uPostProcessingTime.getSmoothed() % 1000);
        ImGui::Text("Unaccounted: %6.3f ms", timeLost / 1000.0);

        ImGui::Separator();

        CvarCheckbox("World", r_drawworld);
        ImGui::SameLine();
        CvarCheckbox("Sky", r_drawsky);
        ImGui::SameLine();
        CvarCheckbox("EBO", r_ebo);

        ImGui::End();
    }
}

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
    m_nQuadVbo.create();

    glBindVertexArray(m_nQuadVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_nQuadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
}

void SceneRenderer::recreateFramebuffer() {
    destroyFramebuffer();

    glGenFramebuffers(1, &m_nHdrFramebuffer);

    // Create FP color buffer
    glGenTextures(1, &m_nColorBuffer);
    glBindTexture(GL_TEXTURE_2D, m_nColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_vViewportSize.x, m_vViewportSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create depth buffer (renderbuffer)
    glGenRenderbuffers(1, &m_nRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_nRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_vViewportSize.x, m_vViewportSize.y);

    // Attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, m_nHdrFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_nColorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_nRenderBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("SceneRenderer::recreateFramebuffer(): framebuffer not complete");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneRenderer::destroyFramebuffer() {
    if (m_nHdrFramebuffer) {
        glDeleteFramebuffers(1, &m_nHdrFramebuffer);
        m_nHdrFramebuffer = 0;
    }

    if (m_nColorBuffer) {
        glDeleteTextures(1, &m_nColorBuffer);
        m_nColorBuffer = 0;
    }

    if (m_nRenderBuffer) {
        glDeleteRenderbuffers(1, &m_nRenderBuffer);
        m_nRenderBuffer = 0;
    }
}

void SceneRenderer::asyncCreateSurfaces() {
    logInfo("Creating surfaces...");
    m_Surf.setLevel(m_pLevel);

    m_Data.surfaces.resize(m_Surf.getSurfaceCount());

    for (size_t i = 0; i < m_Data.surfaces.size(); i++) {
        Surface &surf = m_Data.surfaces[i];
        const SurfaceRenderer::Surface &baseSurf = m_Surf.getSurface(i);
        
        surf.m_nMatIdx = baseSurf.nMatIndex;
        surf.m_iVertexCount = (GLsizei)baseSurf.vVertices.size();

        auto fnGetRandColor = []() { return (rand() % 256) / 255.f; };
        surf.m_Color = {fnGetRandColor(), fnGetRandColor(), fnGetRandColor()};
    }
}

void SceneRenderer::asyncLoadBSPLightmaps() {
    if (m_pLevel->getLightMaps().size() == 0) {
        logInfo("BSP lightmaps: no lightmaps in the map file");
        return;
    }

    logInfo("BSP lightmaps: loading...");
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
            logWarn("BSP lightmaps: face {} has invalid lightmap offset", i);
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
    for (auto &i : sortedLightmaps) {
        Surface &surf = m_Data.surfaces[i.surface];
        const glm::u8vec3 *data = reinterpret_cast<const glm::u8vec3 *>(m_pLevel->getLightMaps().data() + i.offset);
        int x, y;
        
        if (m_pLoadingState->bspLightmapBlock.insert(data, surf.m_BSPLMSize.x, surf.m_BSPLMSize.y, x, y,
                                                     BSP_LIGHTMAP_PADDING)) {
            surf.m_BSPLMOffset.x = x;
            surf.m_BSPLMOffset.y = y;
        } else {
            logWarn("BSP lightmaps: no space for surface {} ({}x{})", i.surface, surf.m_BSPLMSize.x,
                    surf.m_BSPLMSize.y);
        }
    }
}

void SceneRenderer::finishLoadBSPLightmaps() {
    // Load the block into the GPU
    m_Data.bspLightmapBlockTex.create();
    glBindTexture(GL_TEXTURE_2D, m_Data.bspLightmapBlockTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BSP_LIGHTMAP_BLOCK_SIZE, BSP_LIGHTMAP_BLOCK_SIZE, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, m_pLoadingState->bspLightmapBlock.getData());
    glGenerateMipmap(GL_TEXTURE_2D);

    m_pLoadingState->bspLightmapBlock.clear();
    logInfo("BSP lightmaps: loaded.");
}

void SceneRenderer::asyncLoadCustomLightmaps() {
    m_Data.bCustomLMLoaded = false;
    m_pLoadingState->customLightmaps.clear();
    fs::path &lmPath = m_Data.customLightmapPath;

    if (lmPath.empty()) {
        logInfo("Custom lightmaps: file not found - not loading.");
        return;
    }

    logInfo("Custom lightmaps: loading...");

    try {
        constexpr uint32_t LM_MAGIC = ('1' << 24) | ('0' << 16) | ('M' << 8) | ('L' << 0);

        struct LightmapFileHeader {
            uint32_t nMagic;
            uint32_t iFaceCount;
        };

        struct LightmapFaceInfo {
            uint32_t iVertexCount;
            glm::ivec2 lmSize;
        };

        appfw::BinaryReader file(lmPath);

        LightmapFileHeader lmHeader;
        file.read(lmHeader);

        if (lmHeader.nMagic != LM_MAGIC) {
            throw std::runtime_error(fmt::format("Invalid magic: expected {}, got {}", LM_MAGIC, lmHeader.nMagic));
        }

        if (lmHeader.iFaceCount != m_Data.surfaces.size()) {
            throw std::runtime_error(
                fmt::format("Face count mismatch: expected {}, got {}", m_Data.surfaces.size(), lmHeader.iFaceCount));
        }

        // Read face info
        for (size_t i = 0; i < lmHeader.iFaceCount; i++) {
            Surface &surf = m_Data.surfaces[i];
            LightmapFaceInfo info;
            file.read(info);

            if (info.iVertexCount != (uint32_t)surf.m_iVertexCount) {
                throw std::runtime_error("Face vertex count mismatch");
            }

            surf.m_vCustomLMSize = info.lmSize;
            surf.m_vCustomLMTexCoords.resize(info.iVertexCount);
            file.readArray(appfw::span(surf.m_vCustomLMTexCoords));
        }

        m_pLoadingState->customLightmaps.resize(lmHeader.iFaceCount);

        // Read textures
        for (size_t i = 0; i < lmHeader.iFaceCount; i++) {
            Surface &surf = m_Data.surfaces[i];
            std::vector<glm::vec3> &data = m_pLoadingState->customLightmaps[i];
            data.resize((size_t)surf.m_vCustomLMSize.x * (size_t)surf.m_vCustomLMSize.y);
            file.readArray(appfw::span(data));
        }
    } catch (const std::exception &e) {
        logWarn("Custom lightmaps: failed to load: {}", e.what());
        m_pLoadingState->customLightmaps.clear();
    }
}

void SceneRenderer::finishLoadCustomLightmaps() {
    if (m_pLoadingState->customLightmaps.empty()) {
        return;
    }

    for (size_t i = 0; i < m_Data.surfaces.size(); i++) {
        Surface &surf = m_Data.surfaces[i];
        std::vector<glm::vec3> &data = m_pLoadingState->customLightmaps[i];

        surf.m_CustomLMTex.create();
        glBindTexture(GL_TEXTURE_2D, surf.m_CustomLMTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surf.m_vCustomLMSize.x, surf.m_vCustomLMSize.y, 0, GL_RGB, GL_FLOAT,
                     data.data());
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    m_Data.bCustomLMLoaded = true;
    logInfo("Custom lightmaps: loaded");
}

void SceneRenderer::asyncCreateSurfaceObjects() {
    logInfo("Surface objects: creating");

    std::vector<SurfaceVertex> vertices;
    m_pLoadingState->allVertices.reserve(bsp::MAX_MAP_VERTS);
    unsigned maxEboSize = 0; // Sum of vertex counts of all surfaces + surface count (to account for primitive restart element)

    m_pLoadingState->surfVertices.resize(m_Data.surfaces.size());

    for (size_t i = 0; i < m_Data.surfaces.size(); i++) {
        Surface &surf = m_Data.surfaces[i];
        const SurfaceRenderer::Surface &baseSurf = m_Surf.getSurface(i);

        vertices.clear();
        vertices.reserve(surf.m_iVertexCount);
        maxEboSize += surf.m_iVertexCount + 1;

        glm::vec3 normal = baseSurf.pPlane->vNormal;
        if (baseSurf.iFlags & SURF_PLANEBACK) {
            normal = -normal;
        }

        const bsp::BSPTextureInfo &texInfo = *baseSurf.pTexInfo;
        const Material &material = MaterialManager::get().getMaterial(surf.m_nMatIdx);

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
            if (surf.m_BSPLMSize.x != 0) {
                v.bspLMTexture.s = glm::dot(v.position, texInfo.vS);
                v.bspLMTexture.t = glm::dot(v.position, texInfo.vT);
                v.bspLMTexture += glm::vec2(texInfo.fSShift, texInfo.fTShift);
                v.bspLMTexture -= surf.m_vTextureMins;
                v.bspLMTexture += glm::vec2(BSP_LIGHTMAP_DIVISOR / 2); // Shift by half-texel
                v.bspLMTexture += surf.m_BSPLMOffset * BSP_LIGHTMAP_DIVISOR;
                v.bspLMTexture /= BSP_LIGHTMAP_BLOCK_SIZE * BSP_LIGHTMAP_DIVISOR;
            }

            // Custom lightmap texture
            if (surf.m_CustomLMTex != 0) {
                v.customLMTexture = surf.m_vCustomLMTexCoords[j];
            }

            vertices.push_back(v);

            // Add vertex to world vertex list
            unsigned eboIdx = (unsigned)m_pLoadingState->allVertices.size();
            m_pLoadingState->allVertices.push_back(v);

            if (eboIdx >= std::numeric_limits<uint16_t>::max()) {
                logError("Surface objects: vertex limit reached.");
                throw std::runtime_error("vertex limit reached");
                return;
            }

            surf.m_VertexIndices.push_back((uint16_t)eboIdx);
        }

        auto fnGetRandColor = []() { return (rand() % 256) / 255.f; };
        surf.m_Color = {fnGetRandColor(), fnGetRandColor(), fnGetRandColor()};

        m_pLoadingState->surfVertices[i] = std::move(vertices);
    }

    m_pLoadingState->maxEboSize = maxEboSize;
}

void SceneRenderer::finishCreateSurfaceObjects() {
    for (size_t i = 0; i < m_Data.surfaces.size(); i++) {
        Surface &surf = m_Data.surfaces[i];
        std::vector<SurfaceVertex> &vertices = m_pLoadingState->surfVertices[i];
        surf.m_Vao.create();
        surf.m_Vbo.create();

        AFW_ASSERT(vertices.size() == (size_t)surf.m_iVertexCount);

        glBindVertexArray(surf.m_Vao);
        glBindBuffer(GL_ARRAY_BUFFER, surf.m_Vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(SurfaceVertex) * surf.m_iVertexCount, vertices.data(),
                     GL_STATIC_DRAW);
        enableSurfaceAttribs();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    unsigned maxEboSize = m_pLoadingState->maxEboSize;
    auto &allVertices = m_pLoadingState->allVertices;

    // VAO for world geometry
    m_Data.worldVao.create();
    m_Data.worldVbo.create();
    glBindVertexArray(m_Data.worldVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_Data.worldVbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(SurfaceVertex) * allVertices.size(), allVertices.data(), GL_STATIC_DRAW);
    enableSurfaceAttribs();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Create EBO
    logInfo("Maximum EBO size: {} B", maxEboSize * sizeof(uint16_t));
    logInfo("Vertex count: {}", allVertices.size());
    m_Data.worldEboData.resize(maxEboSize);
    m_Data.worldEbo.create();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Data.worldEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxEboSize * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    logInfo("Surface objects: finished");
}

void SceneRenderer::enableSurfaceAttribs() {
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(SurfaceVertex),
                          reinterpret_cast<void *>(offsetof(SurfaceVertex, position)));
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(SurfaceVertex),
                          reinterpret_cast<void *>(offsetof(SurfaceVertex, normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(SurfaceVertex),
                          reinterpret_cast<void *>(offsetof(SurfaceVertex, texture)));
    glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(SurfaceVertex),
                          reinterpret_cast<void *>(offsetof(SurfaceVertex, bspLMTexture)));
    glVertexAttribPointer(4, 2, GL_FLOAT, false, sizeof(SurfaceVertex),
                          reinterpret_cast<void *>(offsetof(SurfaceVertex, customLMTexture)));
}

void SceneRenderer::loadTextures() {
    auto &ws = m_pLevel->getEntities().getWorldspawn();
    std::string wads = ws.getValue<std::string>("wad", "");
    
    if (wads.empty()) {
        logWarn("Level doesn't reference any WAD files.");
        return;
    }

    size_t oldsep = static_cast<size_t>(0) - 1;
    size_t sep = 0;

    auto fnLoadWad = [&](const std::string &wadpath) {
        if (wadpath.empty()) {
            return;
        }

        // Find last '/' or '\'
        size_t pos = 0;

        size_t slashpos = wadpath.find_last_of('/');
        if (slashpos != wadpath.npos)
            pos = slashpos;

        size_t backslashpos = wadpath.find_last_of('\\');
        if (backslashpos != wadpath.npos && (slashpos == wadpath.npos || backslashpos > slashpos))
            pos = backslashpos;

        // Load the WAD
        std::string wadname = wadpath.substr(pos + 1);
        fs::path path = getFileSystem().findFileOrEmpty(wadname, "assets");

        if (!path.empty()) {
            if (!MaterialManager::get().isWadLoaded(wadname)) {
                logInfo("Loading WAD {}", wadname);
                MaterialManager::get().loadWadFile(path);
            }
        } else {
            logError("WAD {} not found", wadname);
        }
    };

    while ((sep = wads.find(';', sep)) != wads.npos) {
        std::string wadpath = wads.substr(oldsep + 1, sep - oldsep - 1);
        fnLoadWad(wadpath);
        oldsep = sep;
        sep++;
    }

    fnLoadWad(wads.substr(oldsep + 1));
}

void SceneRenderer::loadSkyBox() {
    m_Data.skyboxCubemap.create();
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_Data.skyboxCubemap);

    // Filename suffix for side
    constexpr const char *suffixes[] = {"rt", "lf", "up", "dn", "bk", "ft"};

    // Get sky name
    const bsp::Level::EntityListItem &worldspawn = m_pLevel->getEntities().getWorldspawn();
    std::string skyname = worldspawn.getValue<std::string>("skyname", "desert");

    // Load images
    for (int i = 0; i < 6; i++) {
        // Try TGA first
        fs::path path = getFileSystem().findFileOrEmpty("gfx/env/" + skyname + suffixes[i] + ".tga", "assets");

        if (path.empty()) {
            // Try BMP instead
            path = getFileSystem().findFileOrEmpty("gfx/env/" + skyname + suffixes[i] + ".bmp", "assets");
        }

        if (!path.empty()) {
            int width, height, channelNum;
            uint8_t *data = stbi_load(path.u8string().c_str(), &width, &height, &channelNum, 3);

            if (data) {
                if (i == 2) {
                    // Top sky texture needs to be rotated CCW
                    std::vector<uint8_t> rot = rotateImage90CCW(data, width, height);
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, height, width, 0, GL_RGB,
                                 GL_UNSIGNED_BYTE, rot.data());
                } else if (i == 3) {
                    // Bottom sky texture needs to be rotated CW
                    std::vector<uint8_t> rot = rotateImage90CW(data, width, height);
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, height, width, 0, GL_RGB,
                                 GL_UNSIGNED_BYTE, rot.data());
                } else {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB,
                                 GL_UNSIGNED_BYTE, data);
                }

                stbi_image_free(data);
                continue;
            } else {
                logError("Failed to load {}: ", path.u8string(), stbi_failure_reason());
            }
        } else {
            logError("Sky {}{} not found", skyname, suffixes[i]);
        }

        // Set purple checkerboard sky
        const CheckerboardImage &img = CheckerboardImage::get();
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, img.size, img.size, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     img.data.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
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

void SceneRenderer::prepareHdrFramebuffer() {
    appfw::Timer renderTimer;
    renderTimer.start();

    glBindFramebuffer(GL_FRAMEBUFFER, m_nHdrFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderTimer.stop();
    m_Stats.uSetupTime += (unsigned)renderTimer.elapsedMicroseconds();
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

void SceneRenderer::drawWorldSurfaces() {
    // Prepare list of visible world surfaces
    appfw::Timer bspTimer;
    bspTimer.start();
    m_Surf.setContext(&m_Data.viewContext);
    m_Surf.calcWorldSurfaces();
    bspTimer.stop();
    m_Stats.uWorldBSPTime += (unsigned)bspTimer.elapsedMicroseconds();

    appfw::Timer timer;
    timer.start();

    glEnable(GL_DEPTH_TEST);

    if (m_Data.viewContext.getCulling() != SurfaceRenderer::Cull::None) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(m_Data.viewContext.getCulling() != SurfaceRenderer::Cull::Back ? GL_BACK : GL_FRONT);
    }

    if (r_ebo.getValue()) {
        drawWorldSurfacesIndexed();
    } else {
        drawWorldSurfacesVao();
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    timer.stop();
    m_Stats.uWorldRenderingTime += (unsigned)timer.elapsedMicroseconds();
}

void SceneRenderer::drawWorldSurfacesVao() {
    m_sWorldShader.enable();
    m_sWorldShader.setupUniforms(*this);

    auto &textureChain = m_Data.viewContext.getWorldTextureChain();
    auto &textureChainFrames = m_Data.viewContext.getWorldTextureChainFrames();
    unsigned frame = m_Data.viewContext.getWorldTextureChainFrame();
    unsigned drawnSurfs = 0;

    // Bind lightmap block texture
    if (r_lighting.getValue() == 2) {
        if (m_Data.bspLightmapBlockTex != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_Data.bspLightmapBlockTex);
        }
    }

    for (size_t i = 0; i < textureChain.size(); i++) {
        if (textureChainFrames[i] != frame) {
            continue;
        }

        // Bind material
        const Material &mat = MaterialManager::get().getMaterial(m_Data.surfaces[textureChain[i][0]].m_nMatIdx);
        mat.bindTextures();

        for (unsigned j : textureChain[i]) {
            Surface &surf = m_Data.surfaces[j];

            // Bind lightmap texture
            if (r_lighting.getValue() == 3) {
                if (surf.m_CustomLMTex != 0) {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, surf.m_CustomLMTex);
                }
            }

            if (r_texture.getValue() == 1) {
                m_sWorldShader.setColor(surf.m_Color);
            }
            glBindVertexArray(surf.m_Vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)surf.m_iVertexCount);
        }

        drawnSurfs += (unsigned)textureChain[i].size();
    }

    glBindVertexArray(0);
    m_sWorldShader.disable();

    m_Stats.uRenderedWorldPolys = drawnSurfs;
    m_Stats.uDrawCallCount += drawnSurfs;
}

void SceneRenderer::drawWorldSurfacesIndexed() {
    AFW_ASSERT(!m_Data.worldEboData.empty());

    m_sWorldShader.enable();
    m_sWorldShader.setupUniforms(*this);

    auto &textureChain = m_Data.viewContext.getWorldTextureChain();
    auto &textureChainFrames = m_Data.viewContext.getWorldTextureChainFrames();
    unsigned frame = m_Data.viewContext.getWorldTextureChainFrame();
    unsigned drawnSurfs = 0;

    // Bind lightmap block texture
    if (r_lighting.getValue() == 2) {
        if (m_Data.bspLightmapBlockTex != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_Data.bspLightmapBlockTex);
        }
    }

    // Bind buffers
    glBindVertexArray(m_Data.worldVao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Data.worldEbo);
    glPrimitiveRestartIndex(PRIMITIVE_RESTART_IDX);
    glEnable(GL_PRIMITIVE_RESTART);

    for (size_t i = 0; i < textureChain.size(); i++) {
        if (textureChainFrames[i] != frame) {
            continue;
        }

        // Bind material
        const Material &mat = MaterialManager::get().getMaterial(m_Data.surfaces[textureChain[i][0]].m_nMatIdx);
        mat.bindTextures();

        // Fill the EBO
        unsigned eboIdx = 0;

        for (unsigned j : textureChain[i]) {
            Surface &surf = m_Data.surfaces[j];
            std::copy(surf.m_VertexIndices.begin(), surf.m_VertexIndices.end(), m_Data.worldEboData.begin() + eboIdx);
            eboIdx += (unsigned)surf.m_VertexIndices.size();
            m_Data.worldEboData[eboIdx] = PRIMITIVE_RESTART_IDX;
            eboIdx++;
        }

        // Decrement EBO size to remove last PRIMITIVE_RESTART_IDX
        eboIdx--;

        // Update EBO
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboIdx * sizeof(uint16_t), m_Data.worldEboData.data());

        // Draw elements
        glDrawElements(GL_TRIANGLE_FAN, eboIdx, GL_UNSIGNED_SHORT, nullptr);

        drawnSurfs += (unsigned)textureChain[i].size();
        m_Stats.uDrawCallCount++;
    }

    glDisable(GL_PRIMITIVE_RESTART);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    m_sWorldShader.disable();
    m_Stats.uRenderedWorldPolys = drawnSurfs;
}

void SceneRenderer::drawSkySurfaces() {
    appfw::Timer timer;
    timer.start();

    if (!m_Data.viewContext.getSkySurfaces().empty()) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        if (m_Data.viewContext.getCulling() != SurfaceRenderer::Cull::None) {
            glEnable(GL_CULL_FACE);
            glFrontFace(GL_CCW);
            glCullFace(m_Data.viewContext.getCulling() != SurfaceRenderer::Cull::Back ? GL_BACK : GL_FRONT);
        }

        m_sSkyShader.enable();
        m_sSkyShader.setupUniforms(*this);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_Data.skyboxCubemap);

        if (r_ebo.getValue()) {
            drawSkySurfacesIndexed();
        } else {
            drawSkySurfacesVao();
        }

        m_sSkyShader.disable();

        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
    }

    timer.stop();
    m_Stats.uSkyRenderingTime += (unsigned)timer.elapsedMicroseconds();
}

void SceneRenderer::drawSkySurfacesVao() {
    for (unsigned i : m_Data.viewContext.getSkySurfaces()) {
        Surface &surf = m_Data.surfaces[i];

        glBindVertexArray(surf.m_Vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)surf.m_iVertexCount);
    }

    glBindVertexArray(0);
    m_Stats.uRenderedSkyPolys = (unsigned)m_Data.viewContext.getSkySurfaces().size();
    m_Stats.uDrawCallCount += m_Stats.uRenderedSkyPolys;
}

void SceneRenderer::drawSkySurfacesIndexed() {
    AFW_ASSERT(!m_Data.worldEboData.empty());

    // Bind buffers
    glBindVertexArray(m_Data.worldVao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Data.worldEbo);
    glPrimitiveRestartIndex(PRIMITIVE_RESTART_IDX);
    glEnable(GL_PRIMITIVE_RESTART);

    // Fill the EBO
    unsigned eboIdx = 0;

    for (unsigned j : m_Data.viewContext.getSkySurfaces()) {
        Surface &surf = m_Data.surfaces[j];
        std::copy(surf.m_VertexIndices.begin(), surf.m_VertexIndices.end(), m_Data.worldEboData.begin() + eboIdx);
        eboIdx += (unsigned)surf.m_VertexIndices.size();
        m_Data.worldEboData[eboIdx] = PRIMITIVE_RESTART_IDX;
        eboIdx++;
    }

    // Decrement EBO size to remove last PRIMITIVE_RESTART_IDX
    eboIdx--;

    // Update EBO
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboIdx * sizeof(uint16_t), m_Data.worldEboData.data());

    // Draw elements
    glDrawElements(GL_TRIANGLE_FAN, eboIdx, GL_UNSIGNED_SHORT, nullptr);

    m_Stats.uRenderedSkyPolys = (unsigned)m_Data.viewContext.getSkySurfaces().size();
    m_Stats.uDrawCallCount++;

    glDisable(GL_PRIMITIVE_RESTART);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SceneRenderer::doPostProcessing() {
    appfw::Timer timer;
    timer.start();

    m_sPostProcessShader.enable();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_nColorBuffer);
    m_sPostProcessShader.setupUniforms();

    glBindVertexArray(m_nQuadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    m_sPostProcessShader.disable();
    
    timer.stop();
    m_Stats.uPostProcessingTime += (unsigned)timer.elapsedMicroseconds();
}
