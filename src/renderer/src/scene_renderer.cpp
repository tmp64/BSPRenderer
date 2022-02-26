#include <appfw/str_utils.h>
#include <renderer/renderer_engine_interface.h>
#include <renderer/scene_renderer.h>
#include <renderer/utils.h>
#include <renderer/scene_shaders.h>
#include <gui_app_base/imgui_controls.h>
#include "lightmap_iface.h"
#include "bsp_lightmap.h"
#include "custom_lightmap.h"
#include "fake_lightmap.h"
#include "world_renderer.h"
#include "brush_renderer.h"

ConVar<bool> r_drawworld("r_drawworld", true, "Draw world surfaces");
ConVar<bool> r_drawsky("r_drawsky", true, "Draw skybox");
ConVar<bool> r_drawents("r_drawents", true, "Draw entities");
ConVar<float> r_gamma("r_gamma", 2.2f, "Screen gamma");
ConVar<float> r_texgamma("r_texgamma", 2.2f, "Texture gamma");
ConVar<int>
    r_texture("r_texture", 2,
              "Which texture should be used:\n  0 - white color, 1 - texture, 2 - random color");
ConVar<int> r_shading("r_shading", 2,
                      "Lighting method:\n  0 - fullbright, 1 - shaded, 2 - lightmaps");
ConVar<int> r_lightmap("r_lightmap", 1, "Used lightmap:\n  0 - BSP, 1 - custom");
ConVar<bool> r_wireframe("r_wireframe", false, "Draw wireframes");
ConVar<bool> r_nosort("r_nosort", false, "Disable transparent entity sorting");
ConVar<bool> r_notrans("r_notrans", false, "Disable entity transparency");
ConVar<bool> r_filter_lm("r_filter_lm", true, "Filter lightmaps");
ConVar<bool> r_patches("r_patches", false, "Draw custom lightmap patches");

static const char *r_shading_values[] = {"Fullbright", "Shaded", "Lightmaps"};
static const char *r_lightmap_values[] = {"BSP", "Custom"};

//----------------------------------------------------------------
// ViewContext
//----------------------------------------------------------------
void SceneRenderer::ViewContext::setPerspective(float fov, float aspect, float zNear, float zFar) {
    m_Type = ProjType::Perspective;

    float fov_x = fov;
    float fov_x_tan = 0;

    if (fov_x < 1.0) {
        fov_x_tan = 1.0;
    } else if (fov_x <= 179.0f) {
        fov_x_tan = tan(fov_x * glm::pi<float>() / 360.f); // TODO:
    } else {
        fov_x_tan = 1.0;
    }

    float fov_y = atan(fov_x_tan / aspect) * 360.f / glm::pi<float>(); // TODO:
    float fov_y_tan = tan(fov_y * glm::pi<float>() / 360.f) * 4.f; // TODO:

    float xMax = fov_y_tan * aspect;
    float xMin = -xMax;

    m_ProjMat = glm::frustum(xMin, xMax, -fov_y_tan, fov_y_tan, zNear, zFar);

    m_flHorFov = fov_x;
    m_flVertFov = fov_y;
    m_flAspect = aspect;
    m_flNearZ = zNear;
    m_flFarZ = zFar;
}

void SceneRenderer::ViewContext::setPerspViewOrigin(const glm::vec3 &origin,
                                                    const glm::vec3 &angles) {
    m_Type = ProjType::Perspective;
    m_vViewOrigin = origin;
    m_vViewAngles = angles;

    m_ViewMat = glm::identity<glm::mat4>();
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(-90.f), {1.0f, 0.0f, 0.0f});
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(90.f), {0.0f, 0.0f, 1.0f});
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(-m_vViewAngles.z), {1.0f, 0.0f, 0.0f});
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(-m_vViewAngles.x), {0.0f, 1.0f, 0.0f});
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(-m_vViewAngles.y), {0.0f, 0.0f, 1.0f});
    m_ViewMat = glm::translate(m_ViewMat, {-m_vViewOrigin.x, -m_vViewOrigin.y, -m_vViewOrigin.z});
}

bool SceneRenderer::ViewContext::cullBox(glm::vec3 mins, glm::vec3 maxs) const {
    if (m_Cull == Cull::None) {
        return false;
    }

    for (const Plane &p : m_Frustum) {
        switch (p.signbits) {
        case 0:
            if (p.vNormal[0] * maxs[0] + p.vNormal[1] * maxs[1] + p.vNormal[2] * maxs[2] <
                p.fDist)
                return true;
            break;
        case 1:
            if (p.vNormal[0] * mins[0] + p.vNormal[1] * maxs[1] + p.vNormal[2] * maxs[2] <
                p.fDist)
                return true;
            break;
        case 2:
            if (p.vNormal[0] * maxs[0] + p.vNormal[1] * mins[1] + p.vNormal[2] * maxs[2] <
                p.fDist)
                return true;
            break;
        case 3:
            if (p.vNormal[0] * mins[0] + p.vNormal[1] * mins[1] + p.vNormal[2] * maxs[2] <
                p.fDist)
                return true;
            break;
        case 4:
            if (p.vNormal[0] * maxs[0] + p.vNormal[1] * maxs[1] + p.vNormal[2] * mins[2] <
                p.fDist)
                return true;
            break;
        case 5:
            if (p.vNormal[0] * mins[0] + p.vNormal[1] * maxs[1] + p.vNormal[2] * mins[2] <
                p.fDist)
                return true;
            break;
        case 6:
            if (p.vNormal[0] * maxs[0] + p.vNormal[1] * mins[1] + p.vNormal[2] * mins[2] <
                p.fDist)
                return true;
            break;
        case 7:
            if (p.vNormal[0] * mins[0] + p.vNormal[1] * mins[1] + p.vNormal[2] * mins[2] <
                p.fDist)
                return true;
            break;
        default:
            return false;
        }
    }
    return false;
}

bool SceneRenderer::ViewContext::cullSurface(const Surface &surface) const {
    if (m_Cull == Cull::None) {
        return false;
    }

    float dist = planeDiff(m_vViewOrigin, *surface.plane);

    if (m_Cull == Cull::Back) {
        // Back face culling
        if (surface.flags & SURF_PLANEBACK) {
            if (dist >= -BACKFACE_EPSILON)
                return true; // wrong side
        } else {
            if (dist <= BACKFACE_EPSILON)
                return true; // wrong side
        }
    } else if (m_Cull == Cull::Front) {
        // Front face culling
        if (surface.flags & SURF_PLANEBACK) {
            if (dist <= BACKFACE_EPSILON)
                return true; // wrong side
        } else {
            if (dist >= -BACKFACE_EPSILON)
                return true; // wrong side
        }
    }

    // Frustum culling
    return cullBox(surface.vMins, surface.vMaxs);
}

void SceneRenderer::ViewContext::setupFrustum() {
    // Build the transformation matrix for the given view angles
    angleVectors(m_vViewAngles, &m_vForward, &m_vRight, &m_vUp);

    // Setup frustum
    // rotate m_vForward right by FOV_X/2 degrees
    m_Frustum[0].vNormal = rotatePointAroundVector(m_vUp, m_vForward, -(90 - m_flHorFov / 2));
    // rotate m_vForward left by FOV_X/2 degrees
    m_Frustum[1].vNormal = rotatePointAroundVector(m_vUp, m_vForward, 90 - m_flHorFov / 2);
    // rotate m_vForward up by FOV_Y/2 degrees
    m_Frustum[2].vNormal = rotatePointAroundVector(m_vRight, m_vForward, 90 - m_flVertFov / 2);
    // rotate m_vForward down by FOV_Y/2 degrees
    m_Frustum[3].vNormal = rotatePointAroundVector(m_vRight, m_vForward, -(90 - m_flVertFov / 2));
    // near clipping plane
    m_Frustum[4].vNormal = m_vForward;

    for (size_t i = 0; i < 5; i++) {
        m_Frustum[i].fDist = glm::dot(m_vViewOrigin, m_Frustum[i].vNormal);
        m_Frustum[i].signbits = signbitsForPlane(m_Frustum[i].vNormal);
    }

    // Far clipping plane
    glm::vec3 farPoint = vectorMA(m_vViewOrigin, m_flFarZ, m_vForward);
    m_Frustum[5].vNormal = -m_vForward;
    m_Frustum[5].fDist = glm::dot(farPoint, m_Frustum[5].vNormal);
    m_Frustum[5].signbits = signbitsForPlane(m_Frustum[5].vNormal);
}

//----------------------------------------------------------------
// SceneRenderer
//----------------------------------------------------------------
SceneRenderer::SceneRenderer(bsp::Level &level, std::string_view path, IRendererEngine &engine)
    : m_Level(level)
    , m_Engine(engine) {
    initSurfaces();
    createBlitQuad();
    createGlobalUniform();
    createLightstyleBuffer();
    createSurfaceBuffers();
    loadLightmaps(path);

    m_pWorldRenderer = std::make_unique<WorldRenderer>(*this);
    m_pBrushRenderer = std::make_unique<BrushRenderer>(*this);
}

SceneRenderer::~SceneRenderer() {
    destroyBackbuffer();
}

void SceneRenderer::setViewportSize(const glm::ivec2 &size) {
    m_vTargetViewportSize = size;
}

void SceneRenderer::renderScene(GLint targetFb, float flSimTime, float flTimeDelta) {
    appfw::Timer renderTimer;
    appfw::Prof prof("Render Scene");
    
    m_Stats = RenderingStats();
    m_uFrameCount++;

    validateSettings();
    frameSetup(flSimTime, flTimeDelta);
    viewRenderingSetup();
    drawWorld();
    drawEntities();
    viewRenderingEnd();

    glBindFramebuffer(GL_FRAMEBUFFER, targetFb);
    postProcessBlit();

    frameEnd();

    m_Stats.flFrameTime = renderTimer.dseconds();
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

        ImGui::Text("World: %u (%u + %u)", m_Stats.uWorldPolys + m_Stats.uSkyPolys,
                    m_Stats.uWorldPolys, m_Stats.uSkyPolys);
        ImGui::Text("Brush ent surfs: %u", m_Stats.uBrushEntPolys);
        ImGui::Text("Draw calls: %u", m_Stats.uDrawCalls);
        ImGui::Text("EBO overflows: %u", m_Stats.uEboOverflows);
        ImGui::Separator();

        ImGui::Text("Entities: %u", m_uVisibleEntCount);
        ImGui::Text("    Solid: %lu", m_SolidEntityList.size());
        ImGui::Text("    Trans: %lu", m_TransEntityList.size());
        ImGui::Separator();

        CvarCheckbox("World", r_drawworld);
        ImGui::SameLine();
        CvarCheckbox("Sky", r_drawsky);
        ImGui::SameLine();
        CvarCheckbox("Wire", r_wireframe);

        CvarCheckbox("Entities", r_drawents);
        ImGui::SameLine();
        CvarCheckbox("Patches", r_patches);
        CvarCheckbox("Filter lightmaps", r_filter_lm);

        int lighting = std::clamp(r_shading.getValue(), 0, (int)std::size(r_shading_values) - 1);

        if (ImGui::BeginCombo("Shading", r_shading_values[lighting])) {
            for (int i = 0; i < (int)std::size(r_shading_values); i++) {
                if (ImGui::Selectable(r_shading_values[i])) {
                    r_shading.setValue(i);
                }
                if (lighting == i) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        int lightmap = std::clamp(r_lightmap.getValue(), 0, (int)std::size(r_lightmap_values) - 1);

        if (ImGui::BeginCombo("Lightmaps", r_lightmap_values[lightmap])) {
            for (int i = 0; i < (int)std::size(r_lightmap_values); i++) {
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
            loadCustomLightmap();
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
    return m_Surfaces[surface].material;
}

#ifdef RENDERER_SUPPORT_TINTING
void SceneRenderer::setSurfaceTint(int surface, glm::vec4 color) {
    Surface &surf = m_Surfaces[surface];
    color.r = pow(color.r, 2.2f);
    color.g = pow(color.g, 2.2f);
    color.b = pow(color.b, 2.2f);
    std::vector<glm::vec4> colors(surf.vertexCount, color);
    m_SurfaceTintBuffer.bind();
    m_SurfaceTintBuffer.update(sizeof(glm::vec4) * surf.vertexOffset,
                               sizeof(glm::vec4) * colors.size(), colors.data());
    m_SurfaceTintBuffer.unbind();
}
#endif

void SceneRenderer::initSurfaces() {
    auto &lvlFaces = m_Level.getFaces();
    auto &lvlSurfEdges = m_Level.getSurfEdges();
    auto &lvlEdges = m_Level.getEdges();
    auto &lvlVertices = m_Level.getVertices();
    m_Surfaces.resize(lvlFaces.size());

    for (size_t i = 0; i < m_Surfaces.size(); i++) {
        Surface &surface = m_Surfaces[i];
        const bsp::BSPFace &face = lvlFaces[i];

        surface.plane = &m_Level.getPlanes().at(face.iPlane);

        // Some flags
        if (face.nPlaneSide) {
            surface.flags |= SURF_PLANEBACK;
        }
        
        // Create vertices and calculate bounds
        surface.vMins = glm::vec3(999999.0f);
        surface.vMaxs = glm::vec3(-999999.0f);
        surface.vOrigin = glm::vec3(0, 0, 0);

        for (int j = 0; j < face.nEdges; j++) {
            if (j == MAX_SIDE_VERTS) {
                printw("Surface {} is too large (exceeded {} vertices)", j, MAX_SIDE_VERTS);
                break;
            }

            glm::vec3 vertex;
            bsp::BSPSurfEdge iEdgeIdx = lvlSurfEdges.at((size_t)face.iFirstEdge + j);

            if (iEdgeIdx > 0) {
                const bsp::BSPEdge &edge = lvlEdges.at(iEdgeIdx);
                vertex = lvlVertices.at(edge.iVertex[0]);
            } else {
                const bsp::BSPEdge &edge = lvlEdges.at(-iEdgeIdx);
                vertex = lvlVertices.at(edge.iVertex[1]);
            }

            surface.faceVertices.push_back(vertex);

            // Add vertex to bounds
            for (int k = 0; k < 3; k++) {
                float val = vertex[k];

                if (val < surface.vMins[k])
                    surface.vMins[k] = val;

                if (val > surface.vMaxs[k])
                    surface.vMaxs[k] = val;
            }

            // Add vertex to origin
            surface.vOrigin += vertex;
        }

        surface.faceVertices.shrink_to_fit();
        surface.vOrigin /= (float)surface.faceVertices.size();

        // Find material
        const bsp::BSPTextureInfo &texInfo = m_Level.getTexInfo().at(face.iTextureInfo);
        const bsp::BSPMipTex &tex = m_Level.getTextures().at(texInfo.iMiptex);
        auto [material, materialIdx] = getMaterialForTexture(tex);
        surface.material = material;
        surface.materialIdx = materialIdx;

        if (!appfw::strncasecmp(tex.szName, "sky", 3)) {
            // Sky surface
            surface.flags |= SURF_DRAWSKY;
        }

        surface.color = glm::vec3(rand() % 256, rand() % 256, rand() % 256) / 255.0f;
    }
}

void SceneRenderer::createBlitQuad() {
    // clang-format off
    const float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // clang-format on

    m_BlitQuadVao.create();
    m_BlitQuadVbo.create(GL_ARRAY_BUFFER, "SceneRenderer: Blit Quad VBO");

    glBindVertexArray(m_BlitQuadVao);
    m_BlitQuadVbo.bind();
    m_BlitQuadVbo.init(sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
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

void SceneRenderer::createLightstyleBuffer() {
    m_LightstyleBuffer.create(GL_TEXTURE_BUFFER, "SceneRenderer: Lightstyles");
    m_LightstyleBuffer.init(sizeof(m_flLightstyleScales), nullptr, GL_DYNAMIC_DRAW);

    m_LightstyleTexture.create();
    glBindTexture(GL_TEXTURE_BUFFER, m_LightstyleTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_LightstyleBuffer.getId());
}

void SceneRenderer::createSurfaceBuffers() {
    AFW_ASSERT(m_uMaxEboSize == 0);
    std::vector<SurfaceVertex> vertexBuffer;
    vertexBuffer.reserve(bsp::MAX_MAP_VERTS);
    
    for (size_t i = 0; i < m_Surfaces.size(); i++) {
        Surface &surf = m_Surfaces[i];

        surf.vertexOffset = (int)vertexBuffer.size();
        surf.vertexCount = (int)surf.faceVertices.size();
        m_uMaxEboSize += surf.vertexCount + 1;

        glm::vec3 normal = surf.plane->vNormal;
        if (surf.flags & SURF_PLANEBACK) {
            normal = -normal;
        }

        const bsp::BSPFace &face = m_Level.getFaces()[i];
        const bsp::BSPTextureInfo &texInfo = m_Level.getTexInfo().at(face.iTextureInfo);

        for (size_t j = 0; j < surf.faceVertices.size(); j++) {
            SurfaceVertex v;

            // Position
            v.position = surf.faceVertices[j];

            // Normal
            v.normal = normal;

            // Texture
            v.texture.s = glm::dot(v.position, texInfo.vS) + texInfo.fSShift;
            v.texture.s /= surf.material->getWide();

            v.texture.t = glm::dot(v.position, texInfo.vT) + texInfo.fTShift;
            v.texture.t /= surf.material->getTall();

            vertexBuffer.push_back(v);
        }
    }

    if (vertexBuffer.size() > MAX_SURF_VERTEX_COUNT) {
        printe("Surface vertex limit of {} reached. There are {} vertices.", MAX_SURF_VERTEX_COUNT,
               vertexBuffer.size());
        throw std::runtime_error("vertex limit reached");
    }

    // Upload
    int64_t surfaceBuffersSize = 0;
    m_SurfaceVertexBuffer.create(GL_ARRAY_BUFFER, "SceneRenderer: Surface vertices");
    m_SurfaceVertexBuffer.init(sizeof(SurfaceVertex) * vertexBuffer.size(), vertexBuffer.data(),
                               GL_STATIC_DRAW);
    surfaceBuffersSize += m_SurfaceVertexBuffer.getMemUsage();
    m_uSurfaceVertexBufferSize = (unsigned)vertexBuffer.size();

#ifdef RENDERER_SUPPORT_TINTING
    // Tinting buffer
    std::vector<glm::vec4> defaultTint(m_uSurfaceVertexBufferSize, glm::vec4(0, 0, 0, 0));
    m_SurfaceTintBuffer.create(GL_ARRAY_BUFFER, "SceneRenderer: Surface tinting");
    m_SurfaceTintBuffer.init(sizeof(glm::vec4) * defaultTint.size(), defaultTint.data(),
                             GL_DYNAMIC_DRAW);
    surfaceBuffersSize += m_SurfaceTintBuffer.getMemUsage();
#endif

    printi("Surfaces: {} vertices, {:.1f} KiB", vertexBuffer.size(), surfaceBuffersSize / 1024.0);
}

void SceneRenderer::loadLightmaps(std::string_view levelPath) {
    // Fake lightmap
    m_pFakeLightmap = std::make_unique<FakeLightmap>(*this);

    // BSP lightmap
    try {
        m_pBSPLightmap = std::make_unique<BSPLightmap>(*this);
    } catch (const std::exception &e) {
        printw("{}", e.what());
    }

    // Custom lightmaps
    m_CustomLightmapPath =
        getFileSystem().findExistingFile(std::string(levelPath) + ".lm", std::nothrow);

    loadCustomLightmap();
}

void SceneRenderer::loadCustomLightmap() {
    if (!m_CustomLightmapPath.empty()) {
        try {
            m_pCustomLightmap = std::make_unique<CustomLightmap>(*this);
        } catch (const std::exception &e) {
            printw("Custom lightmap: failed to load");
            printw("Custom lightmap: {}", e.what());
        }
    }
}

std::pair<Material *, unsigned> SceneRenderer::getMaterialForTexture(const bsp::BSPMipTex &miptex) {
    Material *mat = m_Engine.getMaterial(miptex);

    if (!mat) {
        mat = MaterialSystem::get().getNullMaterial();
    }

    auto it = m_MaterialIndexes.find(mat);

    if (it != m_MaterialIndexes.end()) {
        return *it;
    } else {
        unsigned idx = m_uNextMaterialIndex++;
        m_MaterialIndexes.insert({mat, idx});
        return {mat, idx};
    }
}

void SceneRenderer::validateSettings() {
    // Update lightmap
    ILightmap *oldLightmap = m_pCurrentLightmap;
    ILightmap *newLightmap = nullptr;

    if (r_shading.getValue() == 2) {
        newLightmap = selectLightmap();

        if (!newLightmap) {
            r_shading.setValue(1);
            newLightmap = m_pFakeLightmap.get();
        }
    } else {
        newLightmap = m_pFakeLightmap.get();
    }

    if (newLightmap != oldLightmap) {
        m_pCurrentLightmap = newLightmap;
        updateSurfaceVao();
    }

    // Lightmap filtering
    m_pCurrentLightmap->updateFilter(r_filter_lm.getValue());

    // Update backbuffer
    if (m_vViewportSize != m_vTargetViewportSize) {
        createBackbuffer();
    }
}

SceneRenderer::ILightmap *SceneRenderer::selectLightmap() {
    ILightmap *returnValue = nullptr;
    int targetLightmap = std::clamp(r_lightmap.getValue(), 0, 1);

    if (targetLightmap == 1) {
        if (m_pCustomLightmap) {
            returnValue = m_pCustomLightmap.get();
        } else {
            printw("Custom lightmap not available, falling back to BSP lightmap.");
            targetLightmap = 0;
        }
    }

    if (targetLightmap == 0) {
        if (m_pBSPLightmap) {
            returnValue = m_pBSPLightmap.get();
        } else {
            printw("BSP lightmap not available, falling back to simple shading.");
        }
    }

    // Update r_lightmap if changed
    if (r_lightmap.getValue() != targetLightmap) {
        r_lightmap.setValue(targetLightmap);
    }

    return returnValue;
}

void SceneRenderer::updateSurfaceVao() {
    m_SurfaceVao.create();
    glBindVertexArray(m_SurfaceVao);

    // Common attributes
    m_SurfaceVertexBuffer.bind();
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
    m_pCurrentLightmap->bindVertBuffer();
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(LightmapVertex),
                           reinterpret_cast<void *>(offsetof(LightmapVertex, lightstyle)));
    glVertexAttribPointer(4, 2, GL_FLOAT, false, sizeof(LightmapVertex),
                          reinterpret_cast<void *>(offsetof(LightmapVertex, texture)));

#ifdef RENDERER_SUPPORT_TINTING
    // Tinting
    m_SurfaceTintBuffer.bind();
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, false, sizeof(glm::vec4), reinterpret_cast<void *>(0));
#endif

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SceneRenderer::frameSetup(float flSimTime, float flTimeDelta) {
    appfw::Prof prof("Frame Setup");

    // Seamless cubemap filtering
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Primitive restart
    glPrimitiveRestartIndex(PRIMITIVE_RESTART_IDX);
    glEnable(GL_PRIMITIVE_RESTART);

    // Fill global uniform object
    m_GlobalUniform.mMainProj = m_ViewContext.getProjectionMatrix();
    m_GlobalUniform.mMainView = m_ViewContext.getViewMatrix();
    m_GlobalUniform.vMainViewOrigin = glm::vec4(m_ViewContext.getViewOrigin(), 0);
    m_GlobalUniform.vflParams1.x = r_texgamma.getValue();
    m_GlobalUniform.vflParams1.y = r_gamma.getValue();
    m_GlobalUniform.vflParams1.z = flSimTime;
    m_GlobalUniform.vflParams1.w = flTimeDelta;
    m_GlobalUniform.viParams1.x = r_texture.getValue();
    m_GlobalUniform.viParams1.y = r_shading.getValue();
    m_GlobalUniform.vflParams2.x = m_pCurrentLightmap->getGamma();

    // Upload it to the GPU
    m_GlobalUniformBuffer.bind();
    m_GlobalUniformBuffer.update(0, sizeof(m_GlobalUniform), &m_GlobalUniform);
    glBindBufferBase(GL_UNIFORM_BUFFER, GLOBAL_UNIFORM_BIND, m_GlobalUniformBuffer.getId());

    // Upload lightstyles
    m_LightstyleBuffer.update(0, sizeof(m_flLightstyleScales), m_flLightstyleScales);

    // Setup main view context
    m_ViewContext.setupFrustum();
}

void SceneRenderer::frameEnd() {
    glBindBufferBase(GL_UNIFORM_BUFFER, GLOBAL_UNIFORM_BIND, 0);
    glDisable(GL_PRIMITIVE_RESTART);
    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void SceneRenderer::viewRenderingSetup() {
    appfw::Prof prof("View Setup");

    // Set up backbuffer
    m_HdrBackbuffer.bind(GL_FRAMEBUFFER);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glViewport(0, 0, m_vViewportSize.x, m_vViewportSize.y);

    // Depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Backface culling
    if (m_ViewContext.getCulling() != ViewContext::Cull::None) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW);
        glCullFace(m_ViewContext.getCulling() == ViewContext::Cull::Back ? GL_BACK : GL_FRONT);
    }

    // Bind lightmap
    glActiveTexture(GL_TEXTURE0 + TEX_BRUSH_LIGHTMAP);
    m_pCurrentLightmap->bindTexture();

    // Bind lightstyles texture
    glActiveTexture(GL_TEXTURE0 + TEX_LIGHTSTYLES);
    glBindTexture(GL_TEXTURE_BUFFER, m_LightstyleTexture);

    m_pBrushRenderer->beginRendering();
}

void SceneRenderer::viewRenderingEnd() {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // In case it was changed and wasn't changed back
}

void SceneRenderer::drawWorld() {
    if (!r_drawworld.getValue()) {
        return;
    }

    {
        appfw::Prof prof("World BSP");
        m_pWorldRenderer->getTexturedWorldSurfaces(m_ViewContext,
                                                   m_pWorldRenderer->m_MainWorldSurfList);
    }

    appfw::Prof prof("World Draw");
    m_pWorldRenderer->drawTexturedWorld(m_pWorldRenderer->m_MainWorldSurfList);

    if (r_drawsky.getValue()) {
        m_pWorldRenderer->drawSkybox(m_pWorldRenderer->m_MainWorldSurfList);
    }

    if (r_wireframe.getValue()) {
        m_pWorldRenderer->drawWireframe(m_pWorldRenderer->m_MainWorldSurfList,
                                        r_drawsky.getValue());
    }
}

void SceneRenderer::drawEntities() {
    if (!r_drawents.getValue()) {
        return;
    }

    appfw::Prof prof("Entities");
    drawSolidEntities();
    drawTransEntities();
}

void SceneRenderer::drawSolidEntities() {
    appfw::Prof prof("Solid");

    // Sort opaque entities
    auto sortFn = [this](ClientEntity *const &lhs, ClientEntity *const &rhs) {
        // Sort by model type
        int lhsmodel = (int)lhs->pModel->type;
        int rhsmodel = (int)rhs->pModel->type;

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
        switch (pClent->pModel->type) {
        case ModelType::Brush: {
            m_pBrushRenderer->drawBrushEntity(m_ViewContext, pClent);
            break;
        }
        }
    }

    setRenderMode(kRenderNormal);
    m_Engine.drawNormalTriangles(m_Stats.uDrawCalls);

    setRenderMode(kRenderNormal);
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
                Model *model = ent->pModel;
                if (model->type == ModelType::Brush) {
                    glm::vec3 avg = (model->vMins + model->vMaxs) * 0.5f;
                    return ent->vOrigin + avg;
                } else {
                    return ent->vOrigin;
                }
            };

            glm::vec3 org1 = fnGetOrigin(lhs) - m_ViewContext.getViewOrigin();
            glm::vec3 org2 = fnGetOrigin(rhs) - m_ViewContext.getViewOrigin();
            float dist1 = glm::length2(org1);
            float dist2 = glm::length2(org2);

            return dist1 > dist2;
        };

        std::sort(m_TransEntityList.begin(), m_TransEntityList.end(), sortFn);
    }

    for (ClientEntity *pClent : m_TransEntityList) {
        switch (pClent->pModel->type) {
        case ModelType::Brush: {
            if (r_nosort.getValue()) {
                // Draw unsorted
                m_pBrushRenderer->drawBrushEntity(m_ViewContext, pClent);
            } else {
                // Whether to sort depends on render mode
                switch (pClent->iRenderMode) {
                case kRenderTransTexture:
                case kRenderTransAdd:
                    m_pBrushRenderer->drawSortedBrushEntity(m_ViewContext, pClent);
                    break;
                default:
                    m_pBrushRenderer->drawBrushEntity(m_ViewContext, pClent);
                }
            }
            break;
        }
        }
    }

    setRenderMode(kRenderNormal);
    m_Engine.drawTransTriangles(m_Stats.uDrawCalls);

    setRenderMode(kRenderNormal);
}

void SceneRenderer::postProcessBlit() {
    appfw::Prof prof("Post-Processing");

    ShaderInstance *shaderInstance =
        SceneShaders::Shaders::postprocess.getShaderInstance(SHADER_TYPE_BLIT_IDX);
    shaderInstance->enable(m_uFrameCount);

    glActiveTexture(GL_TEXTURE0);
    m_HdrColorbuffer.bind();

    glBindVertexArray(m_BlitQuadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    m_Stats.uDrawCalls++;
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

void SceneRenderer::createBackbuffer() {
    destroyBackbuffer();
    m_vViewportSize = m_vTargetViewportSize;

    m_HdrColorbuffer.create("SceneRenderer: view color buffer");
    m_HdrColorbuffer.setFilter(TextureFilter::Bilinear);
    m_HdrColorbuffer.setAnisoLevel(0);
    m_HdrColorbuffer.initTexture(GraphicsFormat::RGBA16F, m_vViewportSize.x, m_vViewportSize.y,
                                 false, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    m_HdrDepthbuffer.create("SceneRenderer: view depth buffer");
    m_HdrDepthbuffer.init(GraphicsFormat::Depth24, m_vViewportSize.x, m_vViewportSize.y);

    m_HdrBackbuffer.create("SceneRenderer: HDR backbuffer");
    m_HdrBackbuffer.attachColor(&m_HdrColorbuffer, 0);
    m_HdrBackbuffer.attachDepth(&m_HdrDepthbuffer);

    if (!m_HdrBackbuffer.isComplete()) {
        throw std::logic_error(
            fmt::format("SceneRenderer HDR backbuffer is not complete. Status: {}",
                        (int)m_HdrBackbuffer.checkStatus()));
    }
}

void SceneRenderer::destroyBackbuffer() {
    m_HdrBackbuffer.destroy();
    m_HdrColorbuffer.destroy();
    m_HdrDepthbuffer.destroy();
}
