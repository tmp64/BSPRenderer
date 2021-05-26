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

    /**
     * Returns index of optimized model.
     */
    unsigned getOptModelIdx();

    /**
     * Sets index of optimized model.
     */
    void setOptModelIdx(unsigned idx);

protected:
    ModelType m_Type = ModelType::Bad;
    glm::vec3 m_vMins, m_vMaxs;
    float m_flRadius = 0;

    // Brush model
    int m_iFirstFace = 0;
    int m_iFaceNum = 0;
    unsigned m_nOptBrushIdx = 0;

    friend class WorldState;
};

inline ModelType Model::getType() { return m_Type; }
inline const glm::vec3 &Model::getMins() { return m_vMins; }
inline const glm::vec3 &Model::getMaxs() { return m_vMaxs; }
inline float Model::getRadius() { return m_flRadius; }
inline int Model::getFirstFace() { return m_iFirstFace; }
inline int Model::getFaceNum() { return m_iFaceNum; }
inline unsigned Model::getOptModelIdx() { return m_nOptBrushIdx; }
inline void Model::setOptModelIdx(unsigned idx) { m_nOptBrushIdx = idx; }

#endif
