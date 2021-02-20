#include <appfw/binary_file.h>
#include <renderer/stb_image.h>
#include <renderer/scene_renderer.h>

appfw::console::ConVar<int> r_cull("r_cull", 1,
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
ConVar<int> r_lighting("r_lighting", 3,
                       "Lighting method:\n  0 - fullbright, 1 - shaded, 2 - HL lightmaps, 3 - custom lightmaps");
ConVar<int> r_tonemap("r_tonemap", 2, "HDR tonemapping method:\n  0 - none (clamp to 1), 1 - Reinhard, 2 - exposure");
ConVar<float> r_exposure("r_exposure", 0.5f, "Picture exposure");
ConVar<float> r_skybright("r_skybright", 3.f, "Sky brightness");

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
    m_Brightness.set(r_skybright.getValue());
    m_ViewOrigin.set(scene.m_Data.viewContext.getViewOrigin());
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
    m_Tonemap.set(r_tonemap.getValue());
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

void SceneRenderer::setLevel(bsp::Level *level, const fs::path &bspPath) {
    if (m_pLevel == level) {
        return;
    }

    m_pLevel = level;
    m_Data = LevelData();

    try {
        m_Surf.setLevel(level);
        
        if (level) {
            createSurfaces();
            loadCustomLightmaps(bspPath);
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
    if (m_bNeedRefreshFB) {
        recreateFramebuffer();
    }

    prepareHdrFramebuffer();
    setupViewContext();

    if (!m_Data.bCustomLMLoaded && r_lighting.getValue() == 3) {
        r_lighting.setValue(1);
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

void SceneRenderer::loadCustomLightmaps([[maybe_unused]] const fs::path &bspPath) {
    m_Data.bCustomLMLoaded = false;
    fs::path lmPath = fs::u8path(bspPath.u8string() + ".lm");

    if (!fs::exists(lmPath)) {
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

            surf.m_vLMSize = info.lmSize;
            surf.m_vLMTexCoords.resize(info.iVertexCount);
            file.readArray(appfw::span(surf.m_vLMTexCoords));
        }

        // Read textures
        for (size_t i = 0; i < lmHeader.iFaceCount; i++) {
            Surface &surf = m_Data.surfaces[i];
            std::vector<glm::vec3> data;
            data.resize((size_t)surf.m_vLMSize.x * (size_t)surf.m_vLMSize.y);
            file.readArray(appfw::span(data));

            surf.m_LMTex.create();
            glBindTexture(GL_TEXTURE_2D, surf.m_LMTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surf.m_vLMSize.x, surf.m_vLMSize.y, 0, GL_RGB, GL_FLOAT,
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

            // Lightmap texture
            if (surf.m_LMTex != 0) {
                v.lmTexture = surf.m_vLMTexCoords[j];
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
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(SurfaceVertex),
                              reinterpret_cast<void *>(offsetof(SurfaceVertex, position)));
        glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(SurfaceVertex),
                              reinterpret_cast<void *>(offsetof(SurfaceVertex, normal)));
        glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(SurfaceVertex),
                              reinterpret_cast<void *>(offsetof(SurfaceVertex, texture)));
        glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(SurfaceVertex),
                              reinterpret_cast<void *>(offsetof(SurfaceVertex, lmTexture)));

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
    m_Surf.setContext(&m_Data.viewContext);
    m_Surf.calcWorldSurfaces();

    // Draw visible surfaces
    glEnable(GL_DEPTH_TEST);

    if (m_Data.viewContext.getCulling() != SurfaceRenderer::Cull::None) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(m_Data.viewContext.getCulling() != SurfaceRenderer::Cull::Back ? GL_BACK : GL_FRONT);
    }

    m_sWorldShader.enable();
    m_sWorldShader.setupUniforms(*this);

    for (unsigned i : m_Data.viewContext.getWorldSurfaces()) {
        Surface &surf = m_Data.surfaces[i];
        const Material &mat = MaterialManager::get().getMaterial(surf.m_nMatIdx);
        mat.bindTextures();

        // Bind lightmap texture
        if (surf.m_LMTex != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, surf.m_LMTex);
        }

        m_sWorldShader.setColor(surf.m_Color);
        glBindVertexArray(surf.m_Vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)surf.m_iVertexCount);
    }

    glBindVertexArray(0);
    m_sWorldShader.disable();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void SceneRenderer::drawSkySurfaces() {
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
