#include <graphics/shader_program.h>

ShaderProgram::ShaderProgram() {}

ShaderProgram::ShaderProgram(ShaderProgram &&other) noexcept {
    *this = std::move(other);
}

ShaderProgram::~ShaderProgram() {
    destroy();
}

ShaderProgram &ShaderProgram::operator=(ShaderProgram &&other) noexcept {
    if (&other != this) {
        destroy();
        m_Id = other.m_Id;
        other.m_Id = 0;
    }

    return *this;
}

void ShaderProgram::create() {
    destroy();
    m_Id = glCreateProgram();
}

void ShaderProgram::destroy() {
    if (m_Id != 0) {
        glDeleteProgram(m_Id);
        m_Id = 0;
    }
}

void ShaderProgram::attachShader(const ShaderStage &shader) {
    glAttachShader(m_Id, shader.getId());
}

bool ShaderProgram::link(std::string &log) {
    glLinkProgram(m_Id);

    // Check if successful
    int bSuccess = false;
    glGetProgramiv(m_Id, GL_LINK_STATUS, &bSuccess);

    // Get the log
    GLint logLen = 0;
    glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &logLen);

    if (logLen != 0) {
        log.resize(logLen - 1);
        glGetProgramInfoLog(m_Id, logLen, nullptr, log.data());
    }

    return bSuccess;
}
