#ifndef BSP_LEVEL_H
#define BSP_LEVEL_H
#include <array>
#include <cstdint>
#include <stdexcept>
#include <memory>
#include <appfw/utils.h>
#include <bsp/bsp_types.h>

namespace bsp {

class LevelFormatException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Level {
public:
    /**
     * Constructs an empty level.
     */
    Level() = default;

    /**
     * Loads BSP from supplied data.
     */
    Level(appfw::span<uint8_t> data);

    /**
     * Loads BSP from specified .bsp file.
     */
    Level(const std::string &filename);

    /**
     * Loads BSP from a .bsp file.
     */
    void loadFromFile(const std::string &path);

    /**
     * Loads BSP from supplied contents of .bsp.
     */
    void loadFromBytes(appfw::span<uint8_t> data);

    inline const std::vector<BSPPlane> &getPlanes() const { return m_Planes; }
    inline const std::vector<glm::vec3> &getVertices() const { return m_Vertices; }
    inline const std::vector<BSPNode> &getNodes() const { return m_Nodes; }
    inline const std::vector<BSPFace> &getFaces() const { return m_Faces; }
    inline const std::vector<BSPLeaf> &getLeaves() const { return m_Leaves; }
    inline const std::vector<BSPEdge> &getEdges() const { return m_Edges; }
    inline const std::vector<BSPSurfEdge> &getSurfEdges() const { return m_SurfEdges; }

private:
    std::vector<BSPPlane> m_Planes;
    std::vector<glm::vec3> m_Vertices;
    std::vector<BSPNode> m_Nodes;
    std::vector<BSPFace> m_Faces;
    std::vector<BSPLeaf> m_Leaves;
    std::vector<BSPEdge> m_Edges;
    std::vector<BSPSurfEdge> m_SurfEdges;
};

} // namespace bsp

#endif
