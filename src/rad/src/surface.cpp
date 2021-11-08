#include "types.h"

namespace {

/**
 * Returns a point with coordinates (x, 0, 0) on a plane.
 */
inline glm::vec3 getXPointOnPlane(const bsp::BSPPlane &plane) {
    return {plane.fDist / plane.vNormal.x, 0.f, 0.f};
}

/**
 * Returns a point with coordinates (0, y, 0) on a plane.
 */
inline glm::vec3 getYPointOnPlane(const bsp::BSPPlane &plane) {
    return {0.f, plane.fDist / plane.vNormal.y, 0.f};
}

/**
 * Returns a point with coordinates (0, 0, z) on a plane.
 */
inline glm::vec3 getZPointOnPlane(const bsp::BSPPlane &plane) {
    return {0.f, 0.f, plane.fDist / plane.vNormal.z};
}

/**
 * Returns a projection of a point onto a plane.
 */
inline glm::vec3 projectPointOnPlane(const bsp::BSPPlane &plane, glm::vec3 point) {
    float h = glm::dot(plane.vNormal, point) - plane.fDist;
    return point - plane.vNormal * h;
}

/**
 * Returns a projection of a vector onto a plane. Length is not normalized.
 */
inline glm::vec3 projectVectorOnPlane(const bsp::BSPPlane &plane, glm::vec3 vec) {
    glm::vec3 a = projectPointOnPlane(plane, {0.f, 0.f, 0.f});
    glm::vec3 b = projectPointOnPlane(plane, vec);
    return b - a;
}

std::vector<glm::vec3> getFaceVertices(const bsp::Level &level, const bsp::BSPFace &face) {
    std::vector<glm::vec3> verts;
    auto &lvlVertices = level.getVertices();
    auto &lvlSurfEdges = level.getSurfEdges();

    for (int j = 0; j < face.nEdges; j++) {
        glm::vec3 vertex;
        bsp::BSPSurfEdge iEdgeIdx = lvlSurfEdges.at((size_t)face.iFirstEdge + j);

        if (iEdgeIdx > 0) {
            const bsp::BSPEdge &edge = level.getEdges().at(iEdgeIdx);
            vertex = lvlVertices.at(edge.iVertex[0]);
        } else {
            const bsp::BSPEdge &edge = level.getEdges().at(-iEdgeIdx);
            vertex = lvlVertices.at(edge.iVertex[1]);
        }

        verts.push_back(vertex);
    }

    return verts;
}

constexpr glm::vec2 MINS_INIT = {16384, 16384};
constexpr glm::vec2 MAXS_INIT = {-16384, -16384};

inline void updateMins2D(glm::vec2 &mins, const glm::vec2 &value) {
    mins.x = std::min(mins.x, value.x);
    mins.y = std::min(mins.y, value.y);
}

inline void updateMaxs2D(glm::vec2 &maxs, const glm::vec2 &value) {
    maxs.x = std::max(maxs.x, value.x);
    maxs.y = std::max(maxs.y, value.y);
}

} // namespace

void rad::Plane::load(RadSimImpl &radSim, const bsp::BSPPlane &bspPlane) {
    // Not yet used
    (void)radSim;

    // Copy BSPPlane
    vNormal = glm::normalize(bspPlane.vNormal);
    fDist = bspPlane.fDist;
    nType = bspPlane.nType;

    // Calculate j
    glm::vec3 unprojJ;

    switch (nType) {
    case bsp::PlaneType::PlaneX:
    case bsp::PlaneType::PlaneAnyX:
        unprojJ = {0, 0, 1};
        break;
    case bsp::PlaneType::PlaneY:
    case bsp::PlaneType::PlaneAnyY:
        unprojJ = {0, 0, 1};
        break;
    case bsp::PlaneType::PlaneZ:
    case bsp::PlaneType::PlaneAnyZ:
        unprojJ = {0, 1, 0};
        break;
    default:
        throw std::runtime_error("Map design error: invalid plane type");
    }

    vJ = glm::normalize(projectVectorOnPlane(*this, unprojJ));
    vI = glm::cross(vJ, vNormal);

    // Calculate world origin
    glm::vec3 origin = projectPointOnPlane(*this, {0, 0, 0});
    glm::vec2 originOnPlane = worldToPlane(origin, vI, vJ);
    vWorldOrigin = origin - originOnPlane.x * vI - originOnPlane.y * vJ;
}

void rad::Face::load(RadSimImpl &radSim, const bsp::BSPFace &bspFace) {
    // Copy BSPFace
    iPlane = bspFace.iPlane;
    nPlaneSide = bspFace.nPlaneSide;
    iFirstEdge = bspFace.iFirstEdge;
    nEdges = bspFace.nEdges;
    iTextureInfo = bspFace.iTextureInfo;
    nLightmapOffset = bspFace.nLightmapOffset;
    memcpy(nStyles, bspFace.nStyles, sizeof(nStyles));

    // Face vars
    pPlane = &radSim.m_Planes.at(iPlane);

    if (nPlaneSide) {
        vNormal = -pPlane->vNormal;
    } else {
        vNormal = pPlane->vNormal;
    }

    // Use texture coordinates for face axis
    const bsp::BSPTextureInfo &texInfo = radSim.m_pLevel->getTexInfo().at(bspFace.iTextureInfo);
    vI = glm::normalize(worldToPlane(texInfo.vS, pPlane->vI, pPlane->vJ));
    vJ = glm::normalize(worldToPlane(texInfo.vT, pPlane->vI, pPlane->vJ));

    // Flags
    const bsp::BSPMipTex &mipTex = radSim.m_pLevel->getTextures().at(texInfo.iMiptex);

    if (!strncmp(mipTex.szName, "SKY", 3) || !strncmp(mipTex.szName, "sky", 3)) {
        iFlags |= FACE_SKY;
    }

    // Process vertices
    auto rawVerts = getFaceVertices(*radSim.m_pLevel, bspFace);
    vertices.reserve(rawVerts.size());
    glm::vec2 planeMins = MINS_INIT;
    glm::vec2 planeMaxs = MAXS_INIT;
    glm::vec2 faceMins = MINS_INIT;
    glm::vec2 faceMaxs = MAXS_INIT;
    vPlaneCenter = {0, 0};

    for (glm::vec3 worldPos : rawVerts) {
        Face::Vertex v;
        v.vWorldPos = worldPos;
        v.vPlanePos = worldToPlane(worldPos, pPlane->vI, pPlane->vJ);
        v.vFacePos = planeToFace(v.vPlanePos, vI, vJ);

        updateMins2D(planeMins, v.vPlanePos);
        updateMaxs2D(planeMaxs, v.vPlanePos);
        updateMins2D(faceMins, v.vFacePos);
        updateMaxs2D(faceMaxs, v.vFacePos);

        vPlaneCenter += v.vPlanePos;
        vertices.push_back(v);
    }

    vPlaneCenter /= (float)rawVerts.size();

    flPatchSize = (float)radSim.m_Profile.iBasePatchSize;
}
