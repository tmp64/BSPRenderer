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
    
    //! Sets whether to use AABB for raycasting.
    virtual void setUseAABB(bool state);

    //! Sets offset from entity's origin of AABB.
    virtual void setAABBPos(glm::vec3 pos);

    //! Sets half-size of the AABB.
    virtual void setAABBHalfExtents(glm::vec3 halfExtents);

    //! Sets color of the AABB.
    virtual void setAABBColor(glm::ivec3 color);

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

    inline bool useAABB() { return m_bUseAABB; }
    inline const glm::vec3 &getBaseAABBPos() { return m_BaseAABBPos; }
    inline const glm::vec3 &getBaseAABBHalfExtents() { return m_BaseAABBHalfExtents; }
    inline const glm::ivec3 &getAABBColor() { return m_BaseAABBColor; }
    inline const glm::vec3 &getAABBPos() { return m_AABBPos; }
    inline const glm::vec3 &getAABBHalfExtents() { return m_AABBHalfExtents; }

    inline Model *getModel() { return m_pModel; }

    inline RenderMode getRenderMode() { return m_nRenderMode; }
    inline RenderFx getRenderFx() { return m_nRenderFx; }
    inline int getFxAmount() { return m_iFxAmount; }
    inline const glm::ivec3 &getFxColor() { return m_vFxColor; }

    //! Converts angles (pitch, yaw, roll) into a quaternion.
    static glm::quat anglesToQuat(glm::vec3 angles);

private:
    // Basic entity options
    std::string m_ClassName;
    std::string m_TargetName;
    glm::vec3 m_vOrigin = glm::vec3(0, 0, 0);
    glm::vec3 m_vAngles = glm::vec3(0, 0, 0);
    int m_iEntIndex = 0;
    bsp::EntityKeyValues *m_pLevelKeyValues = nullptr;

    // Base bounding box (without rotation)
    glm::vec3 m_BaseAABBPos = glm::vec3(0, 0, 0);
    glm::vec3 m_BaseAABBHalfExtents = glm::vec3(8);
    glm::ivec3 m_BaseAABBColor = glm::vec3(255, 255, 255);
    bool m_bUseAABB = true;

    // AABB for rendering and raycasting
    glm::vec3 m_AABBPos = glm::vec3(0, 0, 0);
    glm::vec3 m_AABBHalfExtents = glm::vec3(0);

    // Model
    Model *m_pModel = nullptr;

    // Rendering effects
    RenderMode m_nRenderMode = kRenderNormal;
    RenderFx m_nRenderFx = kRenderFxNone;
    int m_iFxAmount = 0;
    glm::ivec3 m_vFxColor = glm::ivec3(0, 0, 0);

    //! Updates the AABB.
    void updateAABB();
};

#endif