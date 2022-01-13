#ifndef GRAPHICS_SHADER_PROGRAM_H
#define GRAPHICS_SHADER_PROGRAM_H
#include <glad/glad.h>
#include <graphics/shader_stage.h>

class ShaderProgram {
public:
    ShaderProgram();
    ShaderProgram(const ShaderProgram &) = delete;
    ShaderProgram(ShaderProgram &&other) noexcept;
    ~ShaderProgram();

    ShaderProgram &operator=(const ShaderProgram &) = delete;
    ShaderProgram &operator=(ShaderProgram &&other) noexcept;

    //! @returns the ID of the shader.
    inline GLuint getId() const { return m_Id; }

    //! Creates the shader program.
    void create();

    //! Destroys the shader program.
    void destroy();

    //! Attaches a shader to the shader program.
    void attachShader(const ShaderStage &shader);

    //! Links the shader program. Attached shaders can be destroyed after linking.
    //! @param  log     The info log
    //! @returns success or not
    bool link(std::string &log);

private:
    GLuint m_Id = 0;
};

#endif
