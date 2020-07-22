#ifndef BSP_EDGES_LUMP_H
#define BSP_EDGES_LUMP_H
#include <appfw/utils.h>
#include <bsp/base_lump.h>
#include <vector>

namespace bsp {

class EdgesLump : public BaseLump {
public:
    virtual void loadLump(appfw::span<uint8_t> lumpData) override;

    inline const std::vector<BSPEdge> getEdges() const { return m_Edges; }

private:
    std::vector<BSPEdge> m_Edges;
};

} // namespace bsp

#endif
