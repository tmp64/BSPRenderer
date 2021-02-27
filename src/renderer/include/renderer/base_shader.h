#ifndef RENDERER_BASESHADER_H
#define RENDERER_BASESHADER_H
#include <appfw/utils.h>
#include <appfw/services.h>
#include <forward_list>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

class UniformCommon;

class BaseShader : appfw::NoCopy {
public:
    class UniformBase : appfw::NoCopy {
    protected:
        UniformBase(BaseShader *pShader, const char *name);
        // virtual ~CUniformCommon();

        BaseShader *m_pShader = nullptr;
        GLuint m_nLocation = 0;
        std::string m_UniformName;

        void loadLocation();

        friend class BaseShader;
    };

    /**
     * Creates a shader.
     */
    BaseShader(const char *title);
    virtual ~BaseShader();

    /**
     * Loads shader from files, compiles and links it.
     * On error throws std::runtime_error
     */
    virtual void create() = 0;

    /**
     * Deletes compiled shader from OpenGL context.
     */
    virtual void destroy();

    const char *getShaderTitle();
    void enable();
    void disable();

protected:
    const char *m_pShaderTitle = nullptr;

    GLuint m_nVertexShaderId = 0, m_nFragShaderId = 0;
    GLuint m_nProgId = 0;

    void createProgram();
    void createVertexShader(const fs::path &filename);
    void createFragmentShader(const fs::path &filename);
    void linkProgram();

    void createVertexShader(const std::string &filepath, const char *tag);
    void createFragmentShader(const std::string &filepath, const char *tag);

private:
    std::vector<BaseShader::UniformBase *> m_UniformList;

    static std::forward_list<BaseShader *> &getUnregItems();

    friend class ShaderManager;
    friend class BaseShader::UniformBase;
};

template <typename T>
class ShaderUniform : BaseShader::UniformBase {
public:
    inline ShaderUniform(BaseShader *pShader, const char *name) : BaseShader::UniformBase(pShader, name) {
        static_assert(appfw::utils::FalseT<T>::value, "Unknown uniform type");
    }
};

template <>
class ShaderUniform<int> : BaseShader::UniformBase {
public:
    inline ShaderUniform(BaseShader *pShader, const char *name) : BaseShader::UniformBase(pShader, name) {}
    inline void set(int val) { glUniform1i(m_nLocation, val); }
};

template <>
class ShaderUniform<float> : BaseShader::UniformBase {
public:
    inline ShaderUniform(BaseShader *pShader, const char *name) : BaseShader::UniformBase(pShader, name) {}
    inline void set(float val) { glUniform1f(m_nLocation, val); }
};

template <>
class ShaderUniform<glm::vec3> : BaseShader::UniformBase {
public:
    inline ShaderUniform(BaseShader *pShader, const char *name) : BaseShader::UniformBase(pShader, name) {}
    inline void set(const glm::vec3 &v) { glUniform3f(m_nLocation, v.x, v.y, v.z); }
};

template <>
class ShaderUniform<glm::vec4> : BaseShader::UniformBase {
public:
    inline ShaderUniform(BaseShader *pShader, const char *name) : BaseShader::UniformBase(pShader, name) {}
    inline void set(const glm::vec4 &v) { glUniform4f(m_nLocation, v.x, v.y, v.z, v.w); }
};

template <>
class ShaderUniform<glm::mat4> : BaseShader::UniformBase {
public:
    inline ShaderUniform(BaseShader *pShader, const char *name) : BaseShader::UniformBase(pShader, name) {}
    inline void set(const glm::mat4 &v) { glUniformMatrix4fv(m_nLocation, 1, false, glm::value_ptr(v)); }
};

#endif // BaseShader_H
