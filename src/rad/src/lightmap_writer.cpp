#include <appfw/timer.h>
#include <app_base/lightmap.h>
#include "lightmap_writer.h"
#include "filters.h"

static float lightmapFilter(float x) {
    return rad::filters::bspline(x);
}

void rad::LightmapWriter::saveLightmap() {
    printn("Sampling lightmaps...");
    appfw::Timer timer;

    m_flLuxelSize = m_RadSim.m_Profile.flLuxelSize;
    m_iOversampleSize = m_RadSim.m_Profile.iOversample;
    m_iBlockSize = m_RadSim.m_Profile.iBlockSize;
    m_iBlockPadding = m_RadSim.m_Profile.iBlockPadding;

    size_t faceCount = m_RadSim.m_Faces.size();
    m_LightmapIdx.resize(faceCount);
    m_Lightmaps.reserve(faceCount);

    for (size_t i = 0; i < faceCount; i++) {
        processFace(i);
    }

    printi("Sample lightmaps: {:.3} s", timer.dseconds());

    createBlock();
    writeLightmapFile();
}

void rad::LightmapWriter::processFace(size_t faceIdx) {
    const Face &face = m_RadSim.m_Faces[faceIdx];

    if (!face.hasLightmap()) {
        return;
    }

    FaceLightmap lm;

    // Calculate lightmap size
    float luxelSize = m_flLuxelSize / face.flLightmapScale;
    glm::vec2 faceRealSize = face.vFaceMaxs - face.vFaceMins;
    lm.vSize.x = (int)(faceRealSize.x / luxelSize) + 1 + 2 * m_iOversampleSize;
    lm.vSize.y = (int)(faceRealSize.y / luxelSize) + 1 + 2 * m_iOversampleSize;

    // Calculate tex coords
    glm::vec2 lmSize = lm.vSize;
    glm::vec2 faceSize = faceRealSize / glm::vec2(luxelSize); //!< Face size in luxels
    glm::vec2 texOffset = (lmSize - faceSize) / 2.0f;
    lm.vFaceOffset = face.vFaceMins - texOffset * luxelSize;
    lm.vTexCoords.resize(face.vertices.size());

    for (size_t i = 0; i < face.vertices.size(); i++) {
        const Face::Vertex &v = face.vertices[i];
        glm::vec2 &uv = lm.vTexCoords[i];

        uv = (v.vFacePos - face.vFaceMins) / glm::vec2(luxelSize);
        uv += texOffset;
    }

    // Lightmap
    lm.lightmapData.resize((size_t)lm.vSize.x * lm.vSize.y);
    sampleLightmap(lm, faceIdx, luxelSize);

    m_LightmapIdx[faceIdx] = m_Lightmaps.size();
    m_Lightmaps.push_back(std::move(lm));
}

void rad::LightmapWriter::sampleLightmap(FaceLightmap &lm, size_t faceIdx, float luxelSize) {
    glm::ivec2 lmSize = lm.vSize;
    bool bSampleNeighbours = m_RadSim.m_Profile.bSampleNeighbours;
    const Face &face = m_RadSim.m_Faces[faceIdx];

    for (int lmy = 0; lmy < lmSize.y; lmy++) {
        glm::vec3 *lmrow = lm.lightmapData.data() + (size_t)lmy * lmSize.x;

        for (int lmx = 0; lmx < lmSize.x; lmx++) {
            glm::vec3 &luxel = lmrow[lmx];
#if 1
            glm::vec2 luxelPos =
                lm.vFaceOffset + (glm::vec2(lmx, lmy) + glm::vec2(0.5f)) * luxelSize;
            glm::vec3 output = glm::vec3(0, 0, 0);
            float weightSum = 0;

            // Filter options
            float maxPixelSize = std::max(face.flPatchSize, luxelSize);
            float filterk = 1.0f / maxPixelSize;
            float radius = FILTER_RADIUS * maxPixelSize;

            // Sample from face
            sampleFace(face, luxelPos, radius, glm::vec2(filterk), output, weightSum, false);

            if (bSampleNeighbours) {
                // Sample from neighbours
                int normalDir = !!face.nPlaneSide;
                auto &neighbours = m_RadSim.m_Planes[face.iPlane].faces;
                glm::vec3 luxelWorldPos = face.faceToWorld(luxelPos);

                for (unsigned neighbourIdx : neighbours) {
                    Face &neighbour = m_RadSim.m_Faces[neighbourIdx];

                    if (neighbourIdx != faceIdx && neighbour.hasLightmap() &&
                        normalDir == !!neighbour.nPlaneSide) {
                        glm::vec2 luxelPosHere = neighbour.worldToFace(luxelWorldPos);
                        sampleFace(neighbour, luxelPosHere, radius, glm::vec2(filterk), output,
                                   weightSum, true);
                    }
                }
            }

            if (weightSum != 0) {
                luxel = output / weightSum;
            } else {
                // TODO:
                luxel = glm::vec3(1, 0, 1);
            }
#else
            // Random color
            luxel.r = (rand() % 255) / 255.0f;
            luxel.g = (rand() % 255) / 255.0f;
            luxel.b = (rand() % 255) / 255.0f;
#endif
        }
    }
}

//! @param  face        The face
//! @param  luxelPos    Position of the center in face coords
//! @param  radius      Half-size of the square
//! @param  filterk     Filter coefficient
//! @param  out         Output sum of colors
//! @prarm  weightSum   Sum of weights
//! @param  checkTrace  Whether to check if there is visibility from luxelPos to sampled patch
void rad::LightmapWriter::sampleFace(const Face &face, glm::vec2 luxelPos, float radius,
                                     glm::vec2 filterk, glm::vec3 &out, float &weightSum,
                                     bool checkTrace) {
    // Check if pos intersects with the face
    glm::vec2 corners[4];
    getCorners(luxelPos, radius * 2.0f, corners);
    glm::vec2 faceAABB[] = {face.vFaceMins, face.vFaceMaxs};
    glm::vec2 luxelAABB[] = {corners[0], corners[3]};

    if (!intersectAABB(faceAABB, luxelAABB)) {
        return;
    }

    PatchIndex count = face.iNumPatches;
    glm::vec3 tracePos = face.faceToWorld(luxelPos) + face.vNormal * TRACE_OFFSET;
    // TODO: tracePos should be clamped to face's edge

    for (PatchIndex i = 0; i < count; i++) {
        PatchRef patch(m_RadSim.m_Patches, face.iFirstPatch + i);

        glm::vec2 d = patch.getFaceOrigin() - luxelPos;
        glm::vec3 patchTracePos = patch.getOrigin() + patch.getNormal() * TRACE_OFFSET;

        if (std::abs(d.x) <= radius && std::abs(d.y) <= radius &&
            (!checkTrace || m_RadSim.traceLine(tracePos, patchTracePos) == bsp::CONTENTS_EMPTY)) {
            float weight = lightmapFilter(d.x * filterk.x) * lightmapFilter(d.y * filterk.y);
#if 1
            // Sample final color
            out += weight * patch.getFinalColor();
#else
            // Sample reflectivity (for debugging)
            out += weight * patch.getReflectivity();
#endif
            weightSum += weight;
        }
    }
}

void rad::LightmapWriter::createBlock() {
    printn("Allocating lightmap block...");
    appfw::Timer timer;

    if (m_iBlockSize % 8 != 0) {
        throw std::runtime_error("block_size must be divisable by 8");
    }

    m_TexBlock.resize(m_iBlockSize, m_iBlockSize);
    size_t idx = 0;

    for (FaceLightmap &lm : m_Lightmaps) {
        glm::ivec2 offset = glm::ivec2(0, 0);
        
        AFW_ASSERT(lm.vSize.x > 0);
        AFW_ASSERT(lm.vSize.y > 0);

        if (!m_TexBlock.insert(lm.lightmapData.data(), lm.vSize.x, lm.vSize.y, offset.x, offset.y,
                               m_iBlockPadding)) {
            throw std::runtime_error(
                fmt::format("Failed to add lightmap {}/{} ({}x{}) to the texture "
                            "block. Please, resize the texture block.",
                            idx + 1, m_Lightmaps.size(), lm.vSize.x, lm.vSize.y));
        }

        lm.vBlockOffset = offset;
        idx++;
    }

    printi("Allocate lightmap block: {:.3} s", timer.dseconds());
}

void rad::LightmapWriter::writeLightmapFile() {
    printn("Writing lightmap...");
    appfw::Timer timer;

    fs::path lmPath = getFileSystem().getFilePath(m_RadSim.getLightmapPath());
    appfw::BinaryOutputFile file(lmPath);
    size_t faceCount = m_RadSim.m_Faces.size();

    // Header
    file.writeBytes(LightmapFileFormat::MAGIC, sizeof(LightmapFileFormat::MAGIC));
    file.writeUInt32((uint32_t)faceCount);                       // Face count
    file.writeUInt32((uint32_t)m_Lightmaps.size());              // Lightmap count
    file.writeInt32(m_iBlockSize);                               // Lightmap texture wide
    file.writeInt32(m_iBlockSize);                               // Lightmap texture tall
    file.writeByte((uint8_t)LightmapFileFormat::Format::RGBF32); // Lightmap texture format

    // Lightmap texture block
    size_t texBlockSize =
        sizeof(*m_TexBlock.getData()) * m_TexBlock.getWide() * m_TexBlock.getTall();
    file.writeBytes(reinterpret_cast<const uint8_t *>(m_TexBlock.getData()), texBlockSize);

    // Face info
    for (size_t i = 0; i < faceCount; i++) {
        const Face &face = m_RadSim.m_Faces[i];
        size_t vertCount = face.vertices.size();
        file.writeUInt32((uint32_t)vertCount);          // Vertex count
        file.writeVec(glm::vec3(face.vFaceI));          // vI
        file.writeVec(glm::vec3(face.vFaceJ));          // vJ
        file.writeVec(glm::vec3(face.vWorldOrigin));    // World position of (0, 0) plane coord.
        file.writeVec(glm::vec2(0.0f, 0.0f));           // Offset of (0, 0) to get to plane coords
        file.writeVec(face.vFaceMaxs - face.vFaceMins); // Face size

        if (face.hasLightmap()) {
            const FaceLightmap &lm = m_Lightmaps[m_LightmapIdx[i]];

            file.writeByte(1);       // Has lightmap
            file.writeVec(lm.vSize); // Lightmap size

            // Lightmap tex coords
            for (size_t j = 0; j < vertCount; j++) {
                file.writeVec(lm.vTexCoords[j] + glm::vec2(lm.vBlockOffset)); // Tex coord in luxels
            }

            // Patches
            file.writeUInt32(face.iNumPatches); // Patch count

            for (PatchIndex j = face.iFirstPatch; j < face.iFirstPatch + face.iNumPatches; j++) {
                PatchRef patch(m_RadSim.m_Patches, j);
                file.writeVec(patch.getFaceOrigin());
                file.writeFloat(patch.getSize());
            }
        } else {
            file.writeByte(0); // No lightmap
        }
    }

    printi("Write lightmap file: {:.3} s", timer.dseconds());
}

void rad::LightmapWriter::getCorners(glm::vec2 point, float size, glm::vec2 corners[4]) {
    size = size / 2.0f;
    corners[0] = point + glm::vec2(-size, -size);
    corners[1] = point + glm::vec2(size, -size);
    corners[2] = point + glm::vec2(-size, size);
    corners[3] = point + glm::vec2(size, size);
}

bool rad::LightmapWriter::intersectAABB(const glm::vec2 b1[2], const glm::vec2 b2[2]) {
    glm::vec2 pos1 = (b1[0] + b1[1]) / 2.0f;
    glm::vec2 pos2 = (b2[0] + b2[1]) / 2.0f;
    glm::vec2 half1 = b1[1] - pos1;
    glm::vec2 half2 = b2[1] - pos2;
    glm::vec2 d = pos2 - pos1;
    return half1.x + half2.x >= std::abs(d.x)
        && half1.y + half2.y >= std::abs(d.y);
}
