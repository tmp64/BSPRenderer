#include <appfw/dbg.h>
#include <fstream>
#include <renderer/base_shader.h>

BaseShader::BaseShader(const char *title) {
    AFW_ASSERT(title && title[0]);
    m_pShaderTitle = title;

    getUnregItems().push_front(this);
}

BaseShader::~BaseShader() { AFW_ASSERT(m_nVertexShaderId == 0 && m_nFragShaderId == 0 && m_nProgId == 0); }

void BaseShader::destroy() {
    if (m_nVertexShaderId) {
        glDeleteShader(m_nVertexShaderId);
        m_nVertexShaderId = 0;
    }

    if (m_nFragShaderId) {
        glDeleteShader(m_nFragShaderId);
        m_nFragShaderId = 0;
    }

    if (m_nProgId) {
        glDeleteProgram(m_nProgId);
        m_nProgId = 0;
    }
}

const char *BaseShader::getShaderTitle() { return m_pShaderTitle; }

void BaseShader::createProgram() {
    AFW_ASSERT(!m_nProgId);
    m_nProgId = glCreateProgram();
}

//---------------------------------------------------------------
// Shader loading
//---------------------------------------------------------------
static void loadFileToString(const fs::path &path, std::string &string) {
    string.clear();

    std::ifstream input(path);
    if (!input.is_open())
        throw std::runtime_error(std::string("open ") + path.u8string() + " failed: " + strerror(errno));

    // Add version
#ifdef GLAD_OPENGL
    string += "#version 330 core\n";
#elif defined(GLAD_OPENGL_ES)
    string += "#version 300 es\n";
#else
#error
#endif
    string += "#line 1\n";

    std::string line;
    while (std::getline(input, line)) {
        string += line + '\n';
    }
    input.close();
}

void BaseShader::createVertexShader(const fs::path &filepath) {
    AFW_ASSERT(m_nProgId);
    AFW_ASSERT(!m_nVertexShaderId);

    std::string shaderCode;
    loadFileToString(filepath, shaderCode);

    m_nVertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    const char *cstr = shaderCode.c_str();
    glShaderSource(m_nVertexShaderId, 1, &cstr, NULL);
    glCompileShader(m_nVertexShaderId);

    int success;
    glGetShaderiv(m_nVertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(m_nVertexShaderId, 512, NULL, infoLog);

        glDeleteShader(m_nVertexShaderId);
        m_nVertexShaderId = 0;

        throw std::runtime_error(std::string("CreateVertexShader error: ") + infoLog);
    }
}

void BaseShader::createFragmentShader(const fs::path &filepath) {
    AFW_ASSERT(m_nProgId);
    AFW_ASSERT(!m_nFragShaderId);

    std::string shaderCode;
    loadFileToString(filepath, shaderCode);

    m_nFragShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    const char *cstr = shaderCode.c_str();
    glShaderSource(m_nFragShaderId, 1, &cstr, NULL);
    glCompileShader(m_nFragShaderId);

    int success;
    glGetShaderiv(m_nFragShaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(m_nFragShaderId, 512, NULL, infoLog);

        glDeleteShader(m_nFragShaderId);
        m_nFragShaderId = 0;

        throw std::runtime_error(std::string("CreateFragmentShader error: ") + infoLog);
    }
}

void BaseShader::linkProgram() {
    AFW_ASSERT(m_nProgId);
    AFW_ASSERT(m_nVertexShaderId);
    AFW_ASSERT(m_nFragShaderId);

    glAttachShader(m_nProgId, m_nVertexShaderId);
    glAttachShader(m_nProgId, m_nFragShaderId);
    glLinkProgram(m_nProgId);
    {
        int success;
        glGetProgramiv(m_nProgId, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(m_nProgId, 512, NULL, infoLog);

            throw std::runtime_error(std::string("LinkProgram error: ") + infoLog);
        }
    }

    glDeleteShader(m_nVertexShaderId);
    glDeleteShader(m_nFragShaderId);
    m_nVertexShaderId = 0;
    m_nFragShaderId = 0;

    // Load uniforms
    for (auto i : m_UniformList)
        i->loadLocation();
}

void BaseShader::createVertexShader(const std::string &filepath, const char *tag) {
    createVertexShader(getFileSystem().findFile(filepath, tag));
}

void BaseShader::createFragmentShader(const std::string &filepath, const char *tag) {
    createFragmentShader(getFileSystem().findFile(filepath, tag));
}

std::forward_list<BaseShader *> &BaseShader::getUnregItems() {
    static std::forward_list<BaseShader *> list;
    return list;
}

void BaseShader::enable() { glUseProgram(m_nProgId); }

void BaseShader::disable() { glUseProgram(0); }

//---------------------------------------------------------------
// CUniformCommon
//---------------------------------------------------------------
BaseShader::UniformBase::UniformBase(BaseShader *pShader, const char *name) {
    m_pShader = pShader;
    m_pShader->m_UniformList.push_back(this);
    m_UniformName = name;
}

// CUniformCommon::~CUniformCommon() {}

void BaseShader::UniformBase::loadLocation() {
    m_nLocation = glGetUniformLocation(m_pShader->m_nProgId, m_UniformName.c_str());
    if (m_nLocation == -1) {
        throw std::runtime_error(std::string("Failed to get uniform ") + m_UniformName);
    }
}
