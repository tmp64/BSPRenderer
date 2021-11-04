#ifndef BASE_ENTITY_H
#define BASE_ENTITY_H
#include <appfw/utils.h>
#include <bsp/level.h>
#include <renderer/model.h>

class BaseEntity : appfw::NoMove {
public:
    BaseEntity();

    void loadKeyValues(const bsp::Level::EntityListItem &item, int idx = -1);

    inline const std::string &getClassName() { return m_ClassName; }
    inline const glm::vec3 &getOrigin() { return m_vOrigin; }
    inline const glm::vec3 &getAngles() { return m_vAngles; }
    inline Model *getModel() { return m_pModel; }
    inline int getRenderMode() { return m_iRenderMode; }
    inline int getRenderFx() { return m_iRenderFx; }
    inline int getFxAmount() { return m_iFxAmount; }
    inline const glm::ivec3 &getFxColor() { return m_vFxColor; }

private:
    std::string m_ClassName;
    glm::vec3 m_vOrigin = glm::vec3(0, 0, 0);
    glm::vec3 m_vAngles = glm::vec3(0, 0, 0);
    Model *m_pModel = nullptr;
    int m_iRenderMode = 0;
    int m_iRenderFx = 0;
    int m_iFxAmount = 0;
    glm::ivec3 m_vFxColor = glm::ivec3(0, 0, 0);
};

#endif