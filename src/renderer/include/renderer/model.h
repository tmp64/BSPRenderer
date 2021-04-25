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

class Model {
public:
    /**
     * Returns type of the model.
     */
    ModelType getType();

    /**
     * Returns bounding box when entity is not rotated (0, 0, 0).
     */
    const glm::vec3 &getMins();
    const glm::vec3 &getMaxs();

    /**
     * Returns maximum possible distance from (0,0,0) to a vertex.
     */
    float getRadius();

	/**
     * Brush models:
     * Returns index of first surface.
     */
    int getFirstFace();

    /**
     * Brush models:
     * Returns number of model surfaces.
     */
    int getFaceNum();

protected:
    ModelType m_Type = ModelType::Bad;
    glm::vec3 m_vMins, m_vMaxs;
    float m_flRadius;

    // Brush model
    int m_iFirstFace;
    int m_iFaceNum;

    friend class WorldState;
};

inline ModelType Model::getType() { return m_Type; }
inline const glm::vec3 &Model::getMins() { return m_vMins; }
inline const glm::vec3 &Model::getMaxs() { return m_vMaxs; }
//inline float Model::getRadius() { return m_flRadius; }
inline int Model::getFirstFace() { return m_iFirstFace; }
inline int Model::getFaceNum() { return m_iFaceNum; }

#endif
