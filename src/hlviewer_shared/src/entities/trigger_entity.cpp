#include <hlviewer/entities/trigger_entity.h>

void TriggerEntity::spawn() {
    BaseEntity::spawn();
    setIsTrigger(true);
}

void TriggerEntity::onKeyValuesUpdated() {
    setRenderMode(kRenderTransTexture);
    setFxAmount(80);
}
