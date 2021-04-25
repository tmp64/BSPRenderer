#ifndef RENDERER_CLIENT_ENTITY_H
#define RENDERER_CLIENT_ENTITY_H
#include <bsp/bsp_types.h>
#include <renderer/const.h>
#include <renderer/model.h>

class ClientEntity {
public:
    /**
     * Returns entity position in the world.
     */
    const glm::vec3 &getOrigin() const;

    /**
     * Returns entity rotation angles in degrees (pitch, yaw, roll).
     */
    const glm::vec3 &getAngles() const;

    /**
     * Returns render mode.
     */
    RenderMode getRenderMode() const;

    /**
     * Returns render effect.
     */
    RenderFx getRenderFx() const;

    /**
     * Returns FX Amount [0; 255].
     */
    int getFxAmount() const;

    /**
     * Returns FX Color, each component is in [0; 255].
     */
    glm::ivec3 getFxColor() const;

    /**
     * Returns a pointer to the model.
     */
    Model *getModel() const;

    /**
     * Returns whether the entity is non-transparent.
     */
    bool isOpaque() const;

    /**
     * Returns whether entity is static: opaque, doesn't move, rotate or animate.
     */
    bool isStatic() const;

    /**
     * Returns render mode rank: higher rank must be rendered first, lower second
     */
    int getRenderModeRank() const;

    glm::vec3 m_vOrigin;
    glm::vec3 m_vAngles;
    int m_iRenderMode;
    int m_iRenderFx;
    int m_iFxAmount;
    glm::ivec3 m_vFxColor;
    Model *m_pModel;
};

inline const glm::vec3 &ClientEntity::getOrigin() const { return m_vOrigin; }
inline const glm::vec3 &ClientEntity::getAngles() const { return m_vAngles; }
inline RenderMode ClientEntity::getRenderMode() const { return (RenderMode)m_iRenderMode; }
inline RenderFx ClientEntity::getRenderFx() const { return (RenderFx)m_iRenderFx; }
inline int ClientEntity::getFxAmount() const { return m_iFxAmount; }
inline glm::ivec3 ClientEntity::getFxColor() const { return m_vFxColor; }
inline Model *ClientEntity::getModel() const { return m_pModel; }

inline bool ClientEntity::isOpaque() const {
    if (getRenderMode() == kRenderNormal) {
        return true;
    }

    if (getModel()->getType() == ModelType::Sprite) {
        return false;
    }

    if (getRenderMode() == kRenderTransAlpha && getFxAmount() == 255) {
        return true;
    }

    return false;
}

inline int ClientEntity::getRenderModeRank() const {
    switch (getRenderMode()) {
    case kRenderTransTexture:
        return 1; // draw second
    case kRenderTransAdd:
        return 2; // draw third
    case kRenderGlow:
        return 3; // must be last!
    }
    return 0;
}

#endif
