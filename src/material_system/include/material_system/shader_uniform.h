#ifndef MATERIAL_SYSTEM_SHADER_UNIFORM_H
#define MATERIAL_SYSTEM_SHADER_UNIFORM_H
#include <glad/glad.h>

class ShaderUniform {
public:
    inline void setName(const char *name) { m_pszUniformName = name; }

    inline void loadLocation(GLuint progId) {
        m_nLocation = glGetUniformLocation(progId, m_pszUniformName);
    }

protected:
    const char *m_pszUniformName = nullptr;
    GLint m_nLocation = -2;
};

#endif
