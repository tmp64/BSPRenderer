#ifndef BSP_BASE_LUMP_H
#define BSP_BASE_LUMP_H
#include <appfw/utils.h>
#include <bsp/bsp_types.h>

namespace bsp {

class BaseLump {
public:
    virtual ~BaseLump() = default;
    virtual void loadLump(appfw::span<uint8_t> lumpData) = 0;

protected:
    BaseLump() = default;
};

} // namespace bsp

#endif
