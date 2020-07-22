#ifndef BSP_FACES_LUMP_H
#define BSP_FACES_LUMP_H
#include <appfw/utils.h>
#include <bsp/base_lump.h>
#include <vector>

namespace bsp {

class FacesLump : public BaseLump {
public:
    virtual void loadLump(appfw::span<uint8_t> lumpData) override;

    inline const std::vector<BSPFace> getFaces() const { return m_Faces; }

private:
    std::vector<BSPFace> m_Faces;
};

} // namespace bsp

#endif
