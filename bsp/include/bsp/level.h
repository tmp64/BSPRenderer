#ifndef BSP_LEVEL_H
#define BSP_LEVEL_H
#include <array>
#include <cstdint>
#include <stdexcept>
#include <memory>
#include <appfw/utils.h>
#include <bsp/bsp_types.h>
#include <bsp/base_lump.h>
#include <bsp/planes_lump.h>
#include <bsp/vertices_lump.h>
#include <bsp/nodes_lump.h>
#include <bsp/faces_lump.h>
#include <bsp/leaves_lump.h>
#include <bsp/edges_lump.h>
#include <bsp/surfedges_lump.h>

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
     * Loads BSP from supplied contents of .bsp.
     */
    void loadFromBytes(appfw::span<uint8_t> data);

    inline const std::vector<BSPPlane> getPlanes() { return getPlanesLump().getPlanes(); }
    inline const std::vector<glm::vec3> getVertices() { return getVerticesLump().getVertices(); }
    inline const std::vector<BSPNode> getNodes() { return getNodesLump().getNodes(); }
    inline const std::vector<BSPFace> getFaces() { return getFacesLump().getFaces(); }
    inline const std::vector<BSPLeaf> getLeaves() { return getLeavesLump().getLeaves(); }
    inline const std::vector<BSPEdge> getEdges() { return getEdgesLump().getEdges(); }
    inline const std::vector<BSPSurfEdge> getSurfEdges() { return getSurfedgesLump().getSurfEdges(); }

private:
    std::array<std::unique_ptr<BaseLump>, MAX_BSP_LUMPS> m_Lumps;

    inline const PlanesLump &getPlanesLump() { return *static_cast<PlanesLump *>(m_Lumps[LUMP_PLANES].get()); }
    inline const VerticesLump &getVerticesLump() { return *static_cast<VerticesLump *>(m_Lumps[LUMP_VERTICES].get()); }
    inline const NodesLump &getNodesLump() { return *static_cast<NodesLump *>(m_Lumps[LUMP_NODES].get()); }
    inline const FacesLump &getFacesLump() { return *static_cast<FacesLump *>(m_Lumps[LUMP_FACES].get()); }
    inline const LeavesLump &getLeavesLump() { return *static_cast<LeavesLump *>(m_Lumps[LUMP_LEAVES].get()); }
    inline const EdgesLump &getEdgesLump() { return *static_cast<EdgesLump *>(m_Lumps[LUMP_EDGES].get()); }
    inline const SurfEdgesLump &getSurfedgesLump() {
        return *static_cast<SurfEdgesLump *>(m_Lumps[LUMP_SURFEDGES].get());
    }
};

} // namespace bsp

#endif
