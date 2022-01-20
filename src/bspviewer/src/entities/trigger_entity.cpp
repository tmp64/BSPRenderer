#include "trigger_entity.h"

void TriggerEntity::onKeyValuesUpdated() {
    setRenderMode(kRenderTransTexture);
    setFxAmount(80);
}
