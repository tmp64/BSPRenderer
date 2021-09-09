#ifndef GUI_APP_BASE_VIEW_2D_H
#define GUI_APP_BASE_VIEW_2D_H
#include <gui_app_base/base_view.h>

class BaseView2D : public BaseView {
public:
    BaseView2D();
    void setProjBounds(float left, float right, float bottom, float top);

protected:
    virtual glm::mat4 getProjMat() const override;
    virtual glm::mat4 getViewMat() const override;

    void drawTexture(GLuint texture, glm::vec2 pos, glm::vec2 size, glm::vec2 uv0, glm::vec2 uv1);

private:
    class TextureShader;

    struct TexVertex {
        glm::vec3 pos;
        glm::vec2 tex;
    };

    // Projection settings
    float m_flLeft = 0;
    float m_flRight = 0;
    float m_flBottom = 0;
    float m_flTop = 0;

    // Texture quad
    GLuint m_nQuadVao = 0;
    GLuint m_nQuadVbo = 0;
};

#endif
