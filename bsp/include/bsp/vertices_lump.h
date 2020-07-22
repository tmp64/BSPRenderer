#ifndef BSP_VERTICES_LUMP_H
#define BSP_VERTICES_LUMP_H
#include <appfw/utils.h>
#include <bsp/base_lump.h>
#include <vector>

namespace bsp {

class VerticesLump : public BaseLump {
public:
    virtual void loadLump(appfw::span<uint8_t> lumpData) override;

    inline const std::vector<glm::vec3> getVertices() const { return m_Vertices; }

private:
    std::vector<glm::vec3> m_Vertices;
};

} // namespace bsp

#endif
