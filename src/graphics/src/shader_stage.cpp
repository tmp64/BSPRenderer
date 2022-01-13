#include <graphics/shader_stage.h>

ShaderStage::ShaderStage() {}

ShaderStage::ShaderStage(ShaderStage &&other) noexcept {
    *this = std::move(other);
}

ShaderStage::~ShaderStage() {
    destroy();
}

ShaderStage &ShaderStage::operator=(ShaderStage &&other) noexcept {
    if (&other != this) {
        destroy();

        m_Id = other.m_Id;
        other.m_Id = 0;

        m_Log = std::move(other.m_Log);

        m_bSuccess = other.m_bSuccess;
        other.m_bSuccess = false;
    }

    return *this;
}

void ShaderStage::destroy() {
    if (m_Id != 0) {
        glDeleteShader(m_Id);
        m_Id = 0;
    }
}

void ShaderStage::createVertexShader() {
    destroy();
    m_Id = glCreateShader(GL_VERTEX_SHADER);
}

void ShaderStage::createFragmentShader() {
    destroy();
    m_Id = glCreateShader(GL_VERTEX_SHADER);
}

bool ShaderStage::compile(const std::string &sourceCode) {
    const char *cstr = sourceCode.c_str();
    glShaderSource(m_Id, 1, &cstr, nullptr);
    glCompileShader(m_Id);

    // Check if successful
    int bSuccess = false;
    glGetShaderiv(m_Id, GL_COMPILE_STATUS, &bSuccess);

    // Get the log
    GLint logLen = 0;
    glGetShaderiv(m_Id, GL_INFO_LOG_LENGTH, &logLen);

    if (logLen != 0) {
        m_Log.resize(logLen - 1);
        glGetShaderInfoLog(m_Id, logLen, nullptr, m_Log.data());
    }

    return bSuccess;
}

