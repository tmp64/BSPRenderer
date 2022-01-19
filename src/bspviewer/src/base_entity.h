#ifndef BASE_ENTITY_H
#define BASE_ENTITY_H
#include <appfw/utils.h>
#include <bsp/level.h>
#include <bsp/entity_key_values.h>
#include <renderer/model.h>

class BaseEntity : appfw::NoMove {
public:
    BaseEntity();
    virtual ~BaseEntity();

    //! Initializes the entity. Called by the factory.
    //! @param  entIdx      Entity index in the entity list
    //! @param  kv          KeyValues of the entity to load from. Can be null.
    void initialize(int entIdx, std::string_view className, bsp::EntityKeyValues *kv);

    //! Called after the entity was constructed.
    virtual void spawn();

    //! Called for every entity keyvalue or if it was changed.
    virtual bool parseKeyValue(const std::string &key, const bsp::EntityValue &value);

    //! Called after keyvalues were loaded or updated.
    virtual void onKeyValuesUpdated();

    //! Sets the targetname of the entity.
    virtual void setTargetName(std::string_view str);

    //! Updates the entitiy's origin.
    virtual void setOrigin(glm::vec3 origin);

    //! Updates entity's rotation angles (pitch, yaw, roll in degrees).
    virtual void setAngles(glm::vec3 angles);
    
    //! Sets the model from string.
    virtual void setModel(std::string_view model);

    //! Sets the model.
    virtual void setModel(Model *model);

    //! Sets the render mode.
    virtual void setRenderMode(RenderMode mode);

    //! Sets the render effect.
    virtual void setRenderFx(RenderFx fx);

    //! Sets the render effect amount.
    virtual void setFxAmount(int amount);

    //! Sets the render effect color
    //! @param  color   Color in gamma-space, [0; 255].
    virtual void setFxColor(glm::ivec3 color);

    inline int entindex() { return m_iEntIndex; }
    inline const std::string &getClassName() { return m_ClassName; }
    inline const std::string &getTargetName() { return m_TargetName; }
    inline const glm::vec3 &getOrigin() { return m_vOrigin; }
    inline const glm::vec3 &getAngles() { return m_vAngles; }
    inline Model *getModel() { return m_pModel; }
    inline RenderMode getRenderMode() { return m_nRenderMode; }
    inline RenderFx getRenderFx() { return m_nRenderFx; }
    inline int getFxAmount() { return m_iFxAmount; }
    inline const glm::ivec3 &getFxColor() { return m_vFxColor; }

private:
    // Basic entity options
    std::string m_ClassName;
    std::string m_TargetName;
    glm::vec3 m_vOrigin = glm::vec3(0, 0, 0);
    glm::vec3 m_vAngles = glm::vec3(0, 0, 0);
    int m_iEntIndex = 0;
    bsp::EntityKeyValues *m_pLevelKeyValues = nullptr;

    // Model
    Model *m_pModel = nullptr;

    RenderMode m_nRenderMode = kRenderNormal;
    RenderFx m_nRenderFx = kRenderFxNone;
    int m_iFxAmount = 0;
    glm::ivec3 m_vFxColor = glm::ivec3(0, 0, 0);
};

#endif