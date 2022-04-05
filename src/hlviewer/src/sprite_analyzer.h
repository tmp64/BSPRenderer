#ifndef SPRITE_ANALYZER_H
#define SPRITE_ANALYZER_H
#include <bsp/sprite.h>
#include "dialog_base.h"

class SpriteAnalyzer : public DialogBase {
public:
    SpriteAnalyzer();

protected:
    void showContents() override;

private:
    int m_FileCount = 0;
    int m_OpenCount = 0;
    int m_ByType[5] = {};
    int m_ByFormat[4] = {};
    int m_BySyncType[2] = {};
    glm::ivec2 m_MinSize = glm::ivec2(9999, 9999);
    glm::ivec2 m_MaxSize = glm::ivec2(0, 0);
    int m_MinFrames = 9999;
    int m_MaxFrames = 0;

    void analyzeSprite(const std::string &name, const bsp::Sprite &sprite);
    void showBy(size_t itemCount, const char *lables[], int values[]);
};

#endif
