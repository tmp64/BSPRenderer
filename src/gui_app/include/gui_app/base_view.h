#ifndef GUI_APP_BASE_VIEW_H
#define GUI_APP_BASE_VIEW_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <appfw/utils.h>

class BaseView : appfw::utils::NoCopy {
public:
    BaseView();
    ~BaseView();
    void tick(const char *title, bool *isOpen = nullptr);
    void setCameraPos(glm::vec3 pos);
    void setCameraRot(glm::vec3 rot);

    inline void setDrawAxis(bool state) { m_bDrawAxis = state; }
    inline void setGroundGrid(bool state) { m_bDrawGroundGrid = state; }
    inline void setAutoSizeEnabled() { m_bAutoSize = true; }
    void setForcedSize(glm::ivec2 size);

protected:
    glm::vec3 m_vPos = glm::vec3(0, 0, 0);
    glm::vec3 m_vRot = glm::vec3(0, 0, 0);
    glm::ivec2 m_vViewportSize = {1, 1};
    bool m_bAutoSize = true;

    virtual glm::mat4 getProjMat() const = 0;
    virtual glm::mat4 getViewMat() const = 0;

    virtual void draw() = 0;

    void drawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color);

private:
    class GridShader;

    GLuint m_nFramebuffer = 0;
    GLuint m_nColorBuffer = 0;
    GLuint m_nRenderBuffer = 0;

    GLuint m_nMSAAFramebuffer = 0;
    GLuint m_nMSAAColorBuffer = 0;
    GLuint m_nMSAARenderBuffer = 0;
    int m_iMSAALevel = 0;

    GLuint m_nLineVao = 0, m_nLineVbo = 0;

    bool m_bDrawAxis = false;
    bool m_bDrawGroundGrid = false;

    void updateViewportSize(glm::ivec2);
    void recreateBuffers();
    void destroyBuffers();

    void renderScene();
    void drawAxis();
    void drawGroundGrid();
};

#endif
