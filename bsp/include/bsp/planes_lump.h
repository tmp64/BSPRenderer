#ifndef BSP_PLANES_LUMP_H
#define BSP_PLANES_LUMP_H
#include <vector>
#include <appfw/utils.h>
#include <bsp/base_lump.h>

namespace bsp {

class PlanesLump : public BaseLump {
public:
    virtual void loadLump(appfw::span<uint8_t> lumpData) override;

    inline const std::vector<BSPPlane> getPlanes() const {
        return m_Planes;
    }

private:
    std::vector<BSPPlane> m_Planes;
};

} // namespace bsp

#endif
