#ifndef BRUSH_MODEL_H
#define BRUSH_MODEL_H
#include <renderer/model.h>

class BrushModel : public Model {
public:
    BrushModel();

    glm::vec3 m_vOrigin;
    int32_t m_iHeadnodes[bsp::MAX_MAP_HULLS];
    int32_t m_nVisLeafs;
};

#endif
