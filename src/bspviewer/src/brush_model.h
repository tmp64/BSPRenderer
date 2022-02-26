#ifndef BRUSH_MODEL_H
#define BRUSH_MODEL_H
#include <renderer/model.h>

struct BrushModel : public Model {
    BrushModel();

    glm::vec3 vOrigin;
    int32_t iHeadnodes[bsp::MAX_MAP_HULLS];
    int32_t nVisLeafs;
};

#endif
