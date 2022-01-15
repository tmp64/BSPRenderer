#ifndef MATERIAL_SYSTEM_SHADER_INSTANCE_H
#define MATERIAL_SYSTEM_SHADER_INSTANCE_H
#include <appfw/utils.h>
#include <graphics/shader_program.h>

class Shader;

class ShaderInstance : appfw::NoMove {
public:
    ShaderInstance(std::unique_ptr<Shader> &shader);

    //! @returns the shader of this instance.
    template <typename T>
    inline T &getShader() {
        return static_cast<T &>(*m_pShader);
    }

    //! Enables this shader.
    void enable(unsigned curFrame);

    //! Disable any enabled shader.
    static void disable();

    //! Compiles the shader.
    bool compile();

    //! @returns the currently enabled shader.
    static inline ShaderInstance *getEnabledShader() { return m_spCurrentInstance; }

private:
    std::unique_ptr<Shader> m_pShader;
    ShaderProgram m_Prog;
    unsigned m_uLastEnableFrame = 0; //! Last frame that the shader was enabled.

    bool compileShader(ShaderStage &stage, std::string_view vpath,
                       std::string_view globalDefines, std::string_view defines);

    static inline ShaderInstance *m_spCurrentInstance = nullptr;
};

#endif
