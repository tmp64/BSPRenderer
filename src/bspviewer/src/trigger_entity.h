#ifndef TRIGGER_ENTITY_H
#define TRIGGER_ENTITY_H
#include "base_entity.h"

class TriggerEntity : public BaseEntity {
public:
    void onKeyValuesUpdated() override;
};

#endif
