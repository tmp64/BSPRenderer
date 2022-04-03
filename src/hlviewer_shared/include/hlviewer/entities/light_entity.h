#ifndef LIGHT_ENTITY_H
#define LIGHT_ENTITY_H
#include "base_entity.h"

class LightEntity : public BaseEntity {
public:
    static constexpr unsigned SF_LIGHT_START_OFF = 1 << 0;

    void spawn() override;
    bool parseKeyValue(const std::string &key, const bsp::EntityValue &value) override;
    void showInspector() override;

    void enableLight();
    void disableLight();

    inline bool isToggleable() { return m_iStyle >= 32; }

private:
    bool m_bEnabled = false;
    int m_iStyle = 0;
    std::string m_Pattern;
};

#endif
