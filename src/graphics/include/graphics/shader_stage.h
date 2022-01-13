#ifndef GRAPHICS_SHADER_STAGE_H
#define GRAPHICS_SHADER_STAGE_H
#include <appfw/utils.h>
#include <glad/glad.h>

class ShaderStage {
public:
    ShaderStage();
    ShaderStage(const ShaderStage &) = delete;
    ShaderStage(ShaderStage &&other) noexcept;
    ~ShaderStage();

    ShaderStage &operator=(const ShaderStage &) = delete;
    ShaderStage &operator=(ShaderStage &&) noexcept;

    //! Destroys the shader.
    void destroy();

    //! Creates an instance of a vertex shader.
    void createVertexShader();

    //! Creates an instance of a fragment shader.
    void createFragmentShader();

    //! @returns the ID of the shader.
    inline GLuint getId() const { return m_Id; }

    //! @returns the compilation log.
    inline const std::string &getCompileLog() const { return m_Log; }

    //! @returns whether the compilation succeeded.
    inline bool isSuccess() const { return m_bSuccess; }

    //! Compiles the shader.
    //! @returns whether the compilation succeeded.
    bool compile(const std::string &sourceCode);

private:
    GLuint m_Id = 0;
    std::string m_Log;
    bool m_bSuccess = false;
};

#endif
