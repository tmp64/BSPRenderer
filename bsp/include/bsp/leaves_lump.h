#ifndef BSP_LEAVES_LUMP_H
#define BSP_LEAVES_LUMP_H
#include <appfw/utils.h>
#include <bsp/base_lump.h>
#include <vector>

namespace bsp {

class LeavesLump : public BaseLump {
public:
    virtual void loadLump(appfw::span<uint8_t> lumpData) override;

    inline const std::vector<BSPLeaf> getLeaves() const { return m_Leaves; }

private:
    std::vector<BSPLeaf> m_Leaves;
};

} // namespace bsp

#endif
