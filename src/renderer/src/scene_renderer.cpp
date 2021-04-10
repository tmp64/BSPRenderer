#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <renderer/stb_image.h>
#include <renderer/scene_renderer.h>
#include <imgui.h>

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

void SceneRenderer::setLevel(bsp::Level *level, const std::string &path, const char *tag) {
    if (m_pLevel == level) {
        return;
    }

    m_pLevel = level;
    m_Data = LevelData();

    try {
        m_Surf.setLevel(level);
        
        if (level) {
            createSurfaces();
            loadBSPLightmaps();
            loadCustomLightmaps(path, tag);
            createSurfaceObjects();
            loadSkyBox();
        }
    } catch (...) {
        m_Surf.setLevel(nullptr);
        m_pLevel = nullptr;
        throw;
    }

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
    m_Stats.clear();
    appfw::Timer renderTimer;
    renderTimer.start();

    if (m_bNeedRefreshFB) {
        recreateFramebuffer();
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

        ImGui::Text("World: %d (%d + %d)", m_Stats.uRenderedWorldPolys + m_Stats.uRenderedSkyPolys,
                    m_Stats.uRenderedWorldPolys, m_Stats.uRenderedSkyPolys);
        ImGui::Separator();

        unsigned timeLost = m_Stats.uTotalRenderingTime.getSmoothed() - m_Stats.uWorldBSPTime.getSmoothed() -
                            m_Stats.uWorldRenderingTime.getSmoothed() - m_Stats.uSkyRenderingTime.getSmoothed();

        ImGui::Text("FPS: %.3f", 1000000.0 / m_Stats.uTotalRenderingTime.getSmoothed());
        ImGui::Text("Total: %2d.%03d ms", m_Stats.uTotalRenderingTime.getSmoothed() / 1000,
                    m_Stats.uTotalRenderingTime.getSmoothed() % 1000);
        ImGui::Text("BSP: %2d.%03d ms", m_Stats.uWorldBSPTime.getSmoothed() / 1000,
                    m_Stats.uWorldBSPTime.getSmoothed() % 1000);
        ImGui::Text("World: %2d.%03d ms", m_Stats.uWorldRenderingTime.getSmoothed() / 1000,
                    m_Stats.uWorldRenderingTime.getSmoothed() % 1000);
        ImGui::Text("Sky: %2d.%03d ms", m_Stats.uSkyRenderingTime.getSmoothed() / 1000,
                    m_Stats.uSkyRenderingTime.getSmoothed() % 1000);
        ImGui::Text("Unaccounted: %2d.%03d ms", timeLost / 1000, timeLost % 1000);

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

void SceneRenderer::createSurfaces() {
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

void SceneRenderer::loadBSPLightmaps() {
    if (m_pLevel->getLightMaps().size() == 0) {
        logInfo("BSP has no embedded lightmaps.");
        return;
    }

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
            logWarn("Face {} has invalid lightmap offset", i);
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

        // Create texture
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        surf.m_BSPLMTex.create();
        glBindTexture(GL_TEXTURE_2D, surf.m_BSPLMTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wide, tall, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     m_pLevel->getLightMaps().data() + offset);
        glGenerateMipmap(GL_TEXTURE_2D);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
}

void SceneRenderer::loadCustomLightmaps(const std::string &path, const char *tag) {
    if (path.empty() || !tag) {
        logInfo("No path to BSP - custom lightmaps not loaded.");
        return;
    }

    m_Data.bCustomLMLoaded = false;
    fs::path lmPath = getFileSystem().findFileOrEmpty(path + ".lm", tag);

    if (lmPath.empty()) {
        logInfo("Custom lightmap file not found - not loading.");
        return;
    }

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

        // Read textures
        for (size_t i = 0; i < lmHeader.iFaceCount; i++) {
            Surface &surf = m_Data.surfaces[i];
            std::vector<glm::vec3> data;
            data.resize((size_t)surf.m_vCustomLMSize.x * (size_t)surf.m_vCustomLMSize.y);
            file.readArray(appfw::span(data));

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
    } catch (const std::exception &e) {
        logWarn("Failed to load custom light map: {}", e.what());
    }
}

void SceneRenderer::createSurfaceObjects() {
    std::vector<SurfaceVertex> vertices;

    for (size_t i = 0; i < m_Data.surfaces.size(); i++) {
        Surface &surf = m_Data.surfaces[i];
        const SurfaceRenderer::Surface &baseSurf = m_Surf.getSurface(i);

        vertices.reserve(surf.m_iVertexCount);

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
            if (surf.m_BSPLMTex != 0) {
                v.bspLMTexture.s = glm::dot(v.position, texInfo.vS);
                v.bspLMTexture.s += texInfo.fSShift;
                v.bspLMTexture.s -= surf.m_vTextureMins.x;
                v.bspLMTexture.s += 8; // Shift by half-texel
                v.bspLMTexture.s /= surf.m_BSPLMSize.x * BSP_LIGHTMAP_DIVISOR;

                v.bspLMTexture.t = glm::dot(v.position, texInfo.vT);
                v.bspLMTexture.t += texInfo.fTShift;
                v.bspLMTexture.t -= surf.m_vTextureMins.y;
                v.bspLMTexture.t += 8; // Shift by half-texel
                v.bspLMTexture.t /= surf.m_BSPLMSize.y * BSP_LIGHTMAP_DIVISOR;
            }

            // Custom lightmap texture
            if (surf.m_CustomLMTex != 0) {
                v.customLMTexture = surf.m_vCustomLMTexCoords[j];
            }

            vertices.push_back(v);
        }

        auto fnGetRandColor = []() { return (rand() % 256) / 255.f; };
        surf.m_Color = {fnGetRandColor(), fnGetRandColor(), fnGetRandColor()};

        surf.m_Vao.create();
        surf.m_Vbo.create();

        glBindVertexArray(surf.m_Vao);
        glBindBuffer(GL_ARRAY_BUFFER, surf.m_Vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(SurfaceVertex) * surf.m_iVertexCount, vertices.data(), GL_STATIC_DRAW);
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

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        vertices.clear();
    }
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
    glBindFramebuffer(GL_FRAMEBUFFER, m_nHdrFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

    // Draw visible surfaces
    glEnable(GL_DEPTH_TEST);

    if (m_Data.viewContext.getCulling() != SurfaceRenderer::Cull::None) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(m_Data.viewContext.getCulling() != SurfaceRenderer::Cull::Back ? GL_BACK : GL_FRONT);
    }

    m_sWorldShader.enable();
    m_sWorldShader.setupUniforms(*this);

    auto &textureChain = m_Data.viewContext.getWorldTextureChain();
    auto &textureChainFrames = m_Data.viewContext.getWorldTextureChainFrames();
    unsigned frame = m_Data.viewContext.getWorldTextureChainFrame();
    unsigned drawnSurfs = 0;

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
            if (r_lighting.getValue() == 2) {
                if (surf.m_BSPLMTex != 0) {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, surf.m_BSPLMTex);
                }
            } else if (r_lighting.getValue() == 3) {
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

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    timer.stop();
    m_Stats.uWorldRenderingTime += (unsigned)timer.elapsedMicroseconds();
    m_Stats.uRenderedWorldPolys = drawnSurfs;
}

void SceneRenderer::drawSkySurfaces() {
    appfw::Timer timer;
    timer.start();

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

    for (unsigned i : m_Data.viewContext.getSkySurfaces()) {
        Surface &surf = m_Data.surfaces[i];

        glBindVertexArray(surf.m_Vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)surf.m_iVertexCount);
    }

    glBindVertexArray(0);
    m_sSkyShader.disable();

    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    timer.stop();
    m_Stats.uSkyRenderingTime += (unsigned)timer.elapsedMicroseconds();
    m_Stats.uRenderedSkyPolys = (unsigned)m_Data.viewContext.getSkySurfaces().size();
}

void SceneRenderer::doPostProcessing() {
    m_sPostProcessShader.enable();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_nColorBuffer);
    m_sPostProcessShader.setupUniforms();

    glBindVertexArray(m_nQuadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    m_sPostProcessShader.disable();
}
