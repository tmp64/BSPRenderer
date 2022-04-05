#ifndef SPRITE_VIEWER_H
#define SPRITE_VIEWER_H
#include <graphics/texture2d.h>
#include <graphics/framebuffer.h>
#include <material_system/material_system.h>
#include <bsp/sprite.h>
#include <hlviewer/assets/sprite_asset.h>
#include "dialog_base.h"

class SpriteViewer : public DialogBase {
public:
    SpriteViewer(std::string_view path);

protected:
    void showContents() override;

private:
    SpriteAssetRef m_pSprite;
    bool m_bPlayAnim = true;
    float m_flFramerate = 10.0f;
    float m_flCurrentFrame = 0.0f;

    // 2D
    MaterialPtr m_p2DMaterial;
    Framebuffer m_2DFramebuffer;
    glm::vec3 m_2DBgColor = glm::vec3(0, 0, 0);
    glm::vec3 m_2DFgColor = glm::vec3(1, 1, 1);
    float m_fl2DScale = 1;
    bool m_b2DFilter = false;

    void showMenuBar();
    void show2D();
    void show3D();
    void showInfo();
    void update2DContents();
};

#endif
