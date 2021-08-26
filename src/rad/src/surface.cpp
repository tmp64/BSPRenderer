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

    vJ = pPlane->vJ;
    vI = glm::cross(vJ, vNormal);
    AFW_ASSERT(floatEquals(glm::length(vI), 1));
    AFW_ASSERT(floatEquals(glm::length(vJ), 1));

    // Vertices
    {
        auto verts = getFaceVertices(*radSim.m_pLevel, bspFace);

        for (glm::vec3 worldPos : verts) {
            Face::Vertex v;
            v.vWorldPos = worldPos;

            // Calculate plane coords
            v.vPlanePos.x = glm::dot(v.vWorldPos, vI);
            v.vPlanePos.y = glm::dot(v.vWorldPos, vJ);

            vertices.push_back(v);
        }

        vertices.shrink_to_fit();
    }

    // Normalize plane pos
    {
        float minX = 999999, minY = 999999;

        for (Face::Vertex &v : vertices) {
            minX = std::min(minX, v.vPlanePos.x);
            minY = std::min(minY, v.vPlanePos.y);
        }

        for (Face::Vertex &v : vertices) {
            v.vPlanePos.x -= minX;
            v.vPlanePos.y -= minY;
        }

        vPlaneOriginInPlaneCoords = {minX, minY};
        vPlaneOrigin =
            vertices[0].vWorldPos - vI * vertices[0].vPlanePos.x - vJ * vertices[0].vPlanePos.y;
    }

    // Calculate plane bounds and center
    {
        float minX = 999999, minY = 999999;
        float maxX = -999999, maxY = -999999;
        planeCenterPoint = glm::vec2(0, 0);

        for (Face::Vertex &v : vertices) {
            minX = std::min(minX, v.vPlanePos.x);
            minY = std::min(minY, v.vPlanePos.y);

            maxX = std::max(maxX, v.vPlanePos.x);
            maxY = std::max(maxY, v.vPlanePos.y);

            planeCenterPoint += v.vPlanePos;
        }

        planeCenterPoint /= (float)vertices.size();

        planeMinBounds.x = minX;
        planeMinBounds.y = minY;

        planeMaxBounds.x = maxX;
        planeMaxBounds.y = maxY;

        AFW_ASSERT(floatEquals(minX, 0));
        AFW_ASSERT(floatEquals(minY, 0));
    }

    flPatchSize = (float)radSim.m_Profile.iBasePatchSize;

    // Check for sky
    {
        const bsp::BSPTextureInfo &texInfo = radSim.m_pLevel->getTexInfo().at(bspFace.iTextureInfo);
        const bsp::BSPMipTex &mipTex = radSim.m_pLevel->getTextures().at(texInfo.iMiptex);

        if (!strncmp(mipTex.szName, "SKY", 3) || !strncmp(mipTex.szName, "sky", 3)) {
            iFlags |= FACE_SKY;
        }
    }
}
