#include <app_base/lightmap.h>
#include "custom_lightmap.h"

SceneRenderer::CustomLightmap::CustomLightmap(SceneRenderer &renderer) {
    appfw::Timer timer;
    appfw::BinaryInputFile file(renderer.m_CustomLightmapPath);

    // Magic
    uint8_t magic[sizeof(LightmapFileFormat::MAGIC)];
    file.readBytes(magic, sizeof(magic));

    if (memcmp(magic, LightmapFileFormat::MAGIC, sizeof(magic))) {
        throw std::runtime_error("Invalid magic");
    }

    // Level & lightmap info
    uint32_t faceCount = file.readUInt32(); // Face count

    if (faceCount != renderer.m_Surfaces.size()) {
        throw std::runtime_error(fmt::format("Face count mismatch: expected {}, got {}",
                                             renderer.m_Surfaces.size(), faceCount));
    }

    file.readUInt32(); // Lightmap count
    glm::ivec2 lightmapBlockSize;
    lightmapBlockSize.x = file.readInt32(); // Lightmap texture wide
    lightmapBlockSize.y = file.readInt32(); // Lightmap texture tall

    LightmapFileFormat::Format format = (LightmapFileFormat::Format)file.readByte();
    if (format != LightmapFileFormat::Format::RGBF32) {
        throw std::runtime_error("Unsupported lightmap format");
    }

    LightmapFileFormat::Compression compression = (LightmapFileFormat::Compression)file.readByte();
    if (compression != LightmapFileFormat::Compression::None) {
        throw std::runtime_error("Unsupported lightmap compression");
    }

    readTexture(file, lightmapBlockSize);

    // Face info
    std::vector<LightmapVertex> vertexBuffer;
    vertexBuffer.reserve(bsp::MAX_MAP_VERTS);
    std::vector<glm::vec3> patchBuffer;
    patchBuffer.reserve(80000);

    for (uint32_t i = 0; i < faceCount; i++) {
        Surface &surf = renderer.m_Surfaces[i];

        uint32_t vertCount = file.readUInt32(); // Vertex count

        if (vertCount != (uint32_t)surf.vertexCount) {
            throw std::runtime_error("Vertex count mismatch");
        }

        glm::vec3 vI = file.readVec<glm::vec3>();           // vI
        glm::vec3 vJ = file.readVec<glm::vec3>();           // vJ
        glm::vec3 vPlaneOrigin = file.readVec<glm::vec3>(); // World position of (0, 0) plane coord.
        file.readVec<glm::vec2>(); // Offset of (0, 0) to get to plane coords
        file.readVec<glm::vec2>(); // Face size

        // Lightstyles
        glm::ivec4 lightstyles = glm::ivec4(255);
        for (int j = 0; j < bsp::NUM_LIGHTSTYLES; j++) {
            lightstyles[j] = file.readByte();
        }

        bool hasLightmap = file.readByte(); // Has lightmap

        if (hasLightmap) {
            glm::ivec2 lmSize = file.readVec<glm::ivec2>(); // Lightmap size

            // Lightmap tex coords
            for (uint32_t j = 0; j < vertCount; j++) {
                LightmapVertex v;

                glm::vec2 texCoords = file.readVec<glm::vec2>(); // Tex coord in luxels
                v.texture = texCoords / glm::vec2(lightmapBlockSize);

                v.lightstyle = lightstyles;

                vertexBuffer.push_back(v);
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

                patchBuffer.push_back(corners[0]);
                patchBuffer.push_back(corners[1]);

                patchBuffer.push_back(corners[1]);
                patchBuffer.push_back(corners[2]);

                patchBuffer.push_back(corners[2]);
                patchBuffer.push_back(corners[3]);

                patchBuffer.push_back(corners[3]);
                patchBuffer.push_back(corners[0]);
            }
        } else {
            // Insert empty vertices
            vertexBuffer.insert(vertexBuffer.end(), vertCount, LightmapVertex());
        }
    }

    // Upload vertex buffer
    AFW_ASSERT_REL(vertexBuffer.size() == renderer.m_uSurfaceVertexBufferSize);
    m_VertBuffer.create(GL_ARRAY_BUFFER, "SceneRenderer: custom lightmap vertices");
    m_VertBuffer.init(sizeof(LightmapVertex) * vertexBuffer.size(), vertexBuffer.data(),
                      GL_STATIC_DRAW);

    // Upload patch buffer
    m_PatchVertBuffer.create(GL_ARRAY_BUFFER, "SceneRenderer: custom lightmap patches");
    m_PatchVertBuffer.init(sizeof(glm::vec3) * patchBuffer.size(), patchBuffer.data(),
                           GL_STATIC_DRAW);

    printi("Custom lightmaps: {} ms", timer.ms());
    printi("Custom lightmaps: block size {}x{}, {:.2f} MiB", lightmapBlockSize.x, lightmapBlockSize.y,
           m_Texture.getMemoryUsage() / 1024.0 / 1024.0);
    printi("Custom lightmaps: {} patch vertices, {:.2f} MiB", patchBuffer.size(),
           patchBuffer.size() * sizeof(glm::vec3) / 1024.0 / 1024.0);
    printi("Custom lightmaps: vertex buffer {:.1f} KiB", m_VertBuffer.getMemUsage() / 1024.0);
}

float SceneRenderer::CustomLightmap::getGamma() {
    // Texture is in linear space
    return 1.0f;
}

void SceneRenderer::CustomLightmap::bindTexture() {
    m_Texture.bind();
}

void SceneRenderer::CustomLightmap::bindVertBuffer() {
    m_VertBuffer.bind();
}

void SceneRenderer::CustomLightmap::updateFilter(bool filterEnabled) {
    TextureFilter filter = filterEnabled ? TextureFilter::Bilinear : TextureFilter::Nearest;

    if (m_Texture.getFilter() != filter) {
        m_Texture.setFilter(filter);
    }
}

void SceneRenderer::CustomLightmap::readTexture(appfw::BinaryInputFile &file, glm::ivec2 size) {
    size_t textureBlockSize = size.x * size.y * bsp::NUM_LIGHTSTYLES;
    std::vector<glm::vec3> lightmapTexture(textureBlockSize);
    file.readBytes(reinterpret_cast<uint8_t *>(lightmapTexture.data()),
                   textureBlockSize * sizeof(glm::vec3));

    m_Texture.create("SceneRenderer: custom lightmap");
    m_Texture.setWrapMode(TextureWrapMode::Clamp);
    m_Texture.setFilter(TextureFilter::Bilinear);
    m_Texture.initTexture(GraphicsFormat::RGB16F, size.x, size.y, bsp::NUM_LIGHTSTYLES, false,
                          GL_RGB, GL_FLOAT, lightmapTexture.data());
}
