#ifndef RENDERER_SPRITE_MODEL_H
#define RENDERER_SPRITE_MODEL_H
#include <bsp/sprite.h>
#include <material_system/material_system.h>
#include <renderer/model.h>

struct SpriteModel {
    bsp::SpriteInfo spriteInfo;
    MaterialPtr spriteMat;
};

#endif
