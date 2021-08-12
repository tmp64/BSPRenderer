#ifndef RENDERER_BASESHADER_H
#define RENDERER_BASESHADER_H
#include <string>
#include <vector>
#include <forward_list>
#include <appfw/utils.h>
#include <appfw/appfw.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <renderer/shader_definitions.h>

class UniformCommon;

class BaseShader : appfw::NoCopy {
public:
    class UniformBase : appfw::NoCopy {
    protected:
        const char *m_pszUniformName;
        GLint m_nLocation = 0;

        friend class BaseShader;
    };

    class UniformBlock : appfw::NoCopy {
    public:

    private:
        const char *m_pszUniformName;
        GLuint m_nIndex = 0;
        GLuint m_nBindingPoint = 0;

        friend class BaseShader;
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

    //! Adds a uniform to the list.
    //! @param  name    Name of the uniform, must be a constant string
    void addUniform(UniformBase &uniform, const char *name);

    //! Adds a uniform block to the list.
    //! @param  name    Name of the uniform, must be a constant string
    void addUniform(UniformBlock &uniform, const char *name, GLuint bindingPoint);

    //! Returns the list of shader definitions applied to all shaders
    inline ShaderDefinitions &getDefinitions() { return m_Defs; }

    //! Returns the list of shader definitions applied to the vertex shader
    inline ShaderDefinitions &getVertDefinitions() { return m_VertDefs; }

    //! Returns the list of shader definitions applied to the fragment shader
    inline ShaderDefinitions &getFragDefinitions() { return m_FragDefs; }

private:
    static constexpr GLsizei COMPILE_LOG_SIZE = 4096;

    std::string_view m_Title;
    std::string_view m_VertexFilePath;
    std::string_view m_FragmentFilePath;

    GLuint m_nVertShaderId = 0, m_nFragShaderId = 0;
    GLuint m_nProgId = 0;

    bool m_bIsReady = false;

    std::vector<BaseShader::UniformBase *> m_UniformList;
    std::vector<BaseShader::UniformBlock *> m_UniformBlockList;
    ShaderDefinitions m_Defs;
    ShaderDefinitions m_VertDefs;
    ShaderDefinitions m_FragDefs;

    //! Creates an empty shader program.
    void createProgram();

    //! Loads the file from path and compiled it as a shader.
    //! Any warnings/errors will be printed in the console.
    //! @returns shader id or 0 on error
    GLuint compileShader(GLenum shaderType, std::string_view path);

    //! Loads the file into a string and processes all #include directives.
    //! @returns final shader code or an empty string on error
    std::string loadShaderFile(GLenum shaderType, std::string_view path);

    //! Links the shader program and destroys individual shaders.
    bool linkProgram();

    //! Saves the location of the uniform from the shader program.
    void loadUniformLocation(UniformBase &uniform);

    //! Saves the index of the block from the shader program.
    bool loadUniformBlockIndex(UniformBlock &uniform);

    static std::forward_list<BaseShader *> &getUnregItems();

    friend class ShaderManager;
};

template <typename T>
class ShaderUniform : public BaseShader::UniformBase {
public:
    inline ShaderUniform() {
        static_assert(appfw::utils::FalseT<T>::value, "Unknown uniform type");
    }
};

template <>
class ShaderUniform<int> : public BaseShader::UniformBase {
public:
    inline void set(int val) { glUniform1i(m_nLocation, val); }
};

template <>
class ShaderUniform<float> : public BaseShader::UniformBase {
public:
    inline void set(float val) { glUniform1f(m_nLocation, val); }
};

template <>
class ShaderUniform<glm::vec3> : public BaseShader::UniformBase {
public:
    inline void set(const glm::vec3 &v) { glUniform3f(m_nLocation, v.x, v.y, v.z); }
};

template <>
class ShaderUniform<glm::vec4> : public BaseShader::UniformBase {
public:
    inline void set(const glm::vec4 &v) { glUniform4f(m_nLocation, v.x, v.y, v.z, v.w); }
};

template <>
class ShaderUniform<glm::mat4> : public BaseShader::UniformBase {
public:
    inline void set(const glm::mat4 &v) { glUniformMatrix4fv(m_nLocation, 1, false, glm::value_ptr(v)); }
};

#endif // BaseShader_H
