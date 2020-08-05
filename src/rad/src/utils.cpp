#include "utils.h"

std::vector<glm::vec3> getFaceVerteces(const bsp::Level &level, const bsp::BSPFace &face) {
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
