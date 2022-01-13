#ifndef MATERIAL_SYSTEM_SHADER_UNIFORM_H
#define MATERIAL_SYSTEM_SHADER_UNIFORM_H
#include <glad/glad.h>

class ShaderUniform {
public:

    inline void setName(const char *name) { m_pszUniformName = name; }

private:
    const char *m_pszUniformName = nullptr;
    GLint m_nLocation = -1;
};

#endif
