#ifndef RENDERER_BASESHADER_H
#define RENDERER_BASESHADER_H
#include <appfw/utils.h>
#include <appfw/appfw.h>
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
    public:
        void loadLocation();

    protected:
        UniformBase(BaseShader *pShader, const char *name);

        BaseShader *m_pShader = nullptr;
        GLuint m_nLocation = 0;
        std::string m_UniformName;
    };

    BaseShader();
    virtual ~BaseShader();

    //! Returns whether the shader can be used or not.
    inline bool isReady() { return m_bIsReady; }

    //! Reloads and recompiles the shader program.
    //! Errors and warnings will be printed to the console.
    //! @returns true on success (no errors occured)
    bool reload();

    //! Destroys the shader program and individual shaders.
    void destroy();

    //! @returns the name of the shader.
    inline std::string_view getTitle() { return m_Title; }

    //! Enables this shader program. This will disable any other shaders.
    void enable();

    //! Disables this shader program.
    void disable();

protected:
    //! Sets the shader title.
    inline void setTitle(std::string_view title) { m_Title = title; }

    //! Sets the virtual path to the vertex shader.
    inline void setVert(std::string_view vertexPath) { m_VertexFilePath = vertexPath; }

    //! Sets the virtual path to the fragment shader.
    inline void setFrag(std::string_view fragmentPath) { m_FragmentFilePath = fragmentPath; }

private:
    static constexpr GLsizei COMPILE_LOG_SIZE = 4096;

    std::string_view m_Title;
    std::string_view m_VertexFilePath;
    std::string_view m_FragmentFilePath;

    GLuint m_nVertShaderId = 0, m_nFragShaderId = 0;
    GLuint m_nProgId = 0;

    bool m_bIsReady = false;

    std::vector<BaseShader::UniformBase *> m_UniformList;

    //! Creates an empty shader program.
    void createProgram();

    //! Loads the file from path and compiled it as a shader.
    //! Any warnings/errors will be printed in the console.
    //! @returns shader id or 0 on error
    GLuint compileShader(GLenum shaderType, std::string_view path);

    //! Links the shader program and destroys individual shaders.
    bool linkProgram();

    static std::forward_list<BaseShader *> &getUnregItems();

    friend class ShaderManager;
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
