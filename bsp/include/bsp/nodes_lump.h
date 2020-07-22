#ifndef BSP_NODES_LUMP_H
#define BSP_NODES_LUMP_H
#include <appfw/utils.h>
#include <bsp/base_lump.h>
#include <vector>

namespace bsp {

class NodesLump : public BaseLump {
public:
    virtual void loadLump(appfw::span<uint8_t> lumpData) override;

    inline const std::vector<BSPNode> getNodes() const { return m_Nodes; }

private:
    std::vector<BSPNode> m_Nodes;
};

} // namespace bsp

#endif
