#include <stb_rect_pack.h>
#include <app_base/bitmap.h>
#include "bsp_lightmap.h"

SceneRenderer::BSPLightmap::BSPLightmap(SceneRenderer &renderer) {
    bsp::Level &level = renderer.m_Level;

    if (level.getLightMaps().size() == 0) {
        throw std::runtime_error("BSP lightmaps: no lightmaps in the map file");
    }

    appfw::Timer timer;
    std::vector<SurfaceData> surfData(renderer.m_Surfaces.size());
    std::vector<stbrp_rect> rects;
    rects.reserve(renderer.m_Surfaces.size());
    int totalLightmapArea = 0;

    for (size_t i = 0; i < renderer.m_Surfaces.size(); i++) {
        const Surface &surf = renderer.m_Surfaces[i];
        const bsp::BSPFace &face = level.getFaces()[i];
        const bsp::BSPTextureInfo &texInfo = level.getTexInfo().at(face.iTextureInfo);
        SurfaceData &data = surfData[i];

        // Calculate texture extents
        glm::vec2 mins = {999999, 999999};
        glm::vec2 maxs = {-99999, -99999};

        for (size_t j = 0; j < surf.faceVertices.size(); j++) {
            glm::vec3 vert = surf.faceVertices[j];

            float vals = glm::dot(vert, texInfo.vS) + texInfo.fSShift;
            mins.s = std::min(mins.s, vals);
            maxs.s = std::max(maxs.s, vals);

            float valt = glm::dot(vert, texInfo.vT) + texInfo.fTShift;
            mins.t = std::min(mins.t, valt);
            maxs.t = std::max(maxs.t, valt);
        }

        // Calculate lightmap texture extents
        glm::vec2 texmins = {floor(mins.x / LIGHTMAP_DIVISOR), floor(mins.y / LIGHTMAP_DIVISOR)};
        glm::vec2 texmaxs = {ceil(maxs.x / LIGHTMAP_DIVISOR), ceil(maxs.y / LIGHTMAP_DIVISOR)};
        data.textureMins = texmins * (float)LIGHTMAP_DIVISOR; // This one used in the engine (I think)
        // data.textureMins = mins; // This one makes more sense. I don't see any difference in-game between them though

        // Calculate size
        int wide = (int)((texmaxs - texmins).x + 1);
        int tall = (int)((texmaxs - texmins).y + 1);
        data.size = {wide, tall};

        // Skip surfaces without lightmaps
        if (face.nLightmapOffset == bsp::NO_LIGHTMAP_OFFSET) {
            continue;
        }

        // Add to pack list
        stbrp_rect rect;
        rect.id = (int)i;
        rect.w = wide + 2 * LIGHTMAP_PADDING;
        rect.h = tall + 2 * LIGHTMAP_PADDING;
        rects.push_back(rect);
        totalLightmapArea += rect.w * rect.h;
    }

    // Estimate texture size
    int squareSize = (int)ceil(totalLightmapArea / (1 - LIGHTMAP_BLOCK_WASTED));
    int textureSize = std::min((int)sqrt(squareSize), MAX_LIGHTMAP_BLOCK_SIZE);

    if (textureSize % 4 != 0) {
        textureSize += (4 - textureSize % 4); // round up so it's divisable by 4
    }

    // Pack lightmaps
    stbrp_context packContext;
    std::vector<stbrp_node> packNodes(2 * textureSize);
    stbrp_init_target(&packContext, textureSize, textureSize, packNodes.data(),
                      (int)packNodes.size());

    stbrp_pack_rects(&packContext, rects.data(), (int)rects.size());

    // Create bitmaps
    Bitmap<glm::u8vec3> bitmaps[bsp::NUM_LIGHTSTYLES];
    for (auto &bm : bitmaps) {
        bm.init(textureSize, textureSize);
    }

    // Add lightmaps to bitmaps
    int failedCount = 0;
    for (const stbrp_rect &rect : rects) {
        if (rect.was_packed) {
            SurfaceData &data = surfData[rect.id];
            const bsp::BSPFace &face = level.getFaces()[rect.id];
            const glm::u8vec3 *pixels = reinterpret_cast<const glm::u8vec3 *>(
                level.getLightMaps().data() + face.nLightmapOffset);

            for (int i = 0; i < bsp::NUM_LIGHTSTYLES; i++) {
                if (face.nStyles[i] == 255) {
                    break;
                }

                bitmaps[i].copyPixels(rect.x, rect.y, data.size.x, data.size.y,
                                      pixels, LIGHTMAP_PADDING);
                pixels += data.size.x * data.size.y;
            }

            data.offset.x = rect.x + LIGHTMAP_PADDING;
            data.offset.y = rect.y + LIGHTMAP_PADDING;
        } else {
            failedCount++;
        }
    }

    if (failedCount != 0) {
        printw("BSP Lightmaps: {} faces failed to pack.", failedCount);
    }

    // Combine bitmaps
    size_t imageArea = textureSize * textureSize;
    std::vector<glm::u8vec3> finalImage(imageArea * bsp::NUM_LIGHTSTYLES);

    for (int i = 0; i < bsp::NUM_LIGHTSTYLES; i++) {
        auto &pixels = bitmaps[i].getPixels();
        std::copy(pixels.begin(), pixels.end(), finalImage.begin() + imageArea * i);
    }

    // Upload to the GPU
    m_Texture.create("SceneRenderer: BSP lightmap block");
    m_Texture.setWrapMode(TextureWrapMode::Clamp);
    m_Texture.setFilter(TextureFilter::Bilinear);
    m_Texture.initTexture(GraphicsFormat::RGB8, textureSize, textureSize, bsp::NUM_LIGHTSTYLES,
                          false, GL_RGB, GL_UNSIGNED_BYTE, finalImage.data());

    // Fill vertex buffer
    std::vector<LightmapVertex> vertexBuffer;
    vertexBuffer.reserve(bsp::MAX_MAP_VERTS);

    for (size_t i = 0; i < renderer.m_Surfaces.size(); i++) {
        const Surface &surf = renderer.m_Surfaces[i];
        const bsp::BSPFace &face = level.getFaces()[i];
        const bsp::BSPTextureInfo &texInfo = level.getTexInfo().at(face.iTextureInfo);
        SurfaceData &data = surfData[i];

        for (glm::vec3 position : surf.faceVertices) {
            LightmapVertex v;

            // Lightstyle
            for (int k = 0; k < bsp::NUM_LIGHTSTYLES; k++) {
                v.lightstyle[k] = face.nStyles[k];
            }

            // Texture
            v.texture.s = glm::dot(position, texInfo.vS);
            v.texture.t = glm::dot(position, texInfo.vT);
            v.texture += glm::vec2(texInfo.fSShift, texInfo.fTShift);
            v.texture -= data.textureMins;
            v.texture += glm::vec2(LIGHTMAP_DIVISOR / 2); // Shift by half-texel
            v.texture += data.offset * LIGHTMAP_DIVISOR;
            v.texture /= m_Texture.getWide() * LIGHTMAP_DIVISOR;

            vertexBuffer.push_back(v);
        }
    }

    // Upload to the GPU
    AFW_ASSERT_REL(vertexBuffer.size() == renderer.m_uSurfaceVertexBufferSize);
    m_VertBuffer.create(GL_ARRAY_BUFFER, "SceneRenderer: BSP lightmap vertices");
    m_VertBuffer.init(sizeof(LightmapVertex) * vertexBuffer.size(), vertexBuffer.data(),
                      GL_STATIC_DRAW);

    printi("BSP lightmaps: {} ms", timer.ms());
    printi("BSP lightmaps: block size {}x{}, {:.2f} MiB", textureSize, textureSize,
           m_Texture.getMemoryUsage() / 1024.0 / 1024.0);
    printi("BSP lightmaps: vertex buffer {:.1f} KiB", m_VertBuffer.getMemUsage() / 1024.0);
}

float SceneRenderer::BSPLightmap::getGamma() {
    return LIGHTMAP_GAMMA;
}

void SceneRenderer::BSPLightmap::bindTexture() {
    m_Texture.bind();
}

void SceneRenderer::BSPLightmap::bindVertBuffer() {
    m_VertBuffer.bind();
}

void SceneRenderer::BSPLightmap::updateFilter(bool filterEnabled) {
    TextureFilter filter = filterEnabled ? TextureFilter::Bilinear : TextureFilter::Nearest;

    if (m_Texture.getFilter() != filter) {
        m_Texture.setFilter(filter);
    }
}
