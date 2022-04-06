#ifndef RENDERER_CLIENT_ENTITY_H
#define RENDERER_CLIENT_ENTITY_H
#include <bsp/bsp_types.h>
#include <renderer/const.h>
#include <renderer/model.h>

class ClientEntity {
public:
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

    glm::vec3 vOrigin;
    glm::vec3 vAngles;
    RenderMode iRenderMode;
    RenderFx iRenderFx;
    int iFxAmount;
    glm::ivec3 vFxColor;
    Model *pModel;
    float flFrame;
    float flScale = 1;
};

inline bool ClientEntity::isOpaque() const {
    if (iRenderMode == kRenderNormal) {
        return true;
    }

    if (pModel->type == ModelType::Sprite) {
        return false;
    }

    if (iRenderMode == kRenderTransAlpha && iFxAmount == 255) {
        return true;
    }

    return false;
}

inline int ClientEntity::getRenderModeRank() const {
    switch (iRenderMode) {
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
