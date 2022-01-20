#include "player_start_entity.h"

void PlayerStartEntity::spawn() {
    setAABBHalfExtents(glm::vec3(32, 32, 72) / 2.0f);
    setAABBColor({0, 224, 0});
}
