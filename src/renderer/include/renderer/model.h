#ifndef RENDERER_MODEL_H
#define RENDERER_MODEL_H
#include <bsp/bsp_types.h>
#include <renderer/const.h>

enum class ModelType {
	Bad = -1,
	Brush,
	Sprite,
	Alias,
	Studio
};

struct Model {
    ModelType type = ModelType::Bad;
    glm::vec3 vMins, vMaxs; //!< AABB
    float flRadius = 0; //!< Maximum dist from origin on an axis

    // Brush model
    unsigned uFirstFace = 0;
    unsigned uFaceNum = 0;
};

#endif
