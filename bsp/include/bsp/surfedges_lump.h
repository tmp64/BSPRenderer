#ifndef BSP_SURFEDGES_LUMP_H
#define BSP_SURFEDGES_LUMP_H
#include <appfw/utils.h>
#include <bsp/base_lump.h>
#include <vector>

namespace bsp {

class SurfEdgesLump : public BaseLump {
public:
    virtual void loadLump(appfw::span<uint8_t> lumpData) override;

    inline const std::vector<BSPSurfEdge> getSurfEdges() const { return m_SurfEdges; }

private:
    std::vector<BSPSurfEdge> m_SurfEdges;
};

} // namespace bsp

#endif
