#include <appfw/dbg.h>
#include <fstream>
#include <renderer/base_shader.h>



//---------------------------------------------------------------
// Shader loading
//---------------------------------------------------------------
static void loadFileToString(const fs::path &path, std::string &string) {
    string.clear();

    std::ifstream input(path);
    if (!input.is_open())
        throw std::runtime_error(std::string("open ") + path.u8string() +
                                 " failed: " + strerror(errno));

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





BaseShader::BaseShader() {
    getUnregItems().push_front(this);
}

BaseShader::~BaseShader() {
    // Shaders are global variables so they must be destroyed by ShaderManager
    // since OpenGL context is not available here
    AFW_ASSERT(m_nVertShaderId == 0 && m_nFragShaderId == 0 && m_nProgId == 0);
}

bool BaseShader::reload() {
    AFW_ASSERT(!m_Title.empty());
    AFW_ASSERT(!m_VertexFilePath.empty());
    AFW_ASSERT(!m_FragmentFilePath.empty());

    if (m_Title.empty()) {
        printwtf("Shader doesn't have a title. That's bad.");
        std::abort();
    }

    destroy();
    createProgram();

    m_nVertShaderId = compileShader(GL_VERTEX_SHADER, m_VertexFilePath);
    m_nFragShaderId = compileShader(GL_FRAGMENT_SHADER, m_FragmentFilePath);

    if (m_nVertShaderId == 0 || m_nFragShaderId == 0) {
        destroy();
        return false;
    }

    if (!linkProgram()) {
        destroy();
        return false;
    }

    m_bIsReady = true;
    return true;
}

void BaseShader::destroy() {
    m_bIsReady = false;

    if (m_nVertShaderId) {
        glDeleteShader(m_nVertShaderId);
        m_nVertShaderId = 0;
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

void BaseShader::enable() {
    glUseProgram(m_nProgId);
}

void BaseShader::disable() {
    glUseProgram(0);
}

void BaseShader::createProgram() {
    AFW_ASSERT(!m_nProgId);
    m_nProgId = glCreateProgram();
}

GLuint BaseShader::compileShader(GLenum shaderType, std::string_view path) {
    fs::path fullPath;

    try {
        fullPath = getFileSystem().findExistingFile(path);
    } catch (...) {
        printe("Shader file {} not found", path);
        return 0;
    }

    std::string shaderCode;
    loadFileToString(fullPath, shaderCode);

    GLuint shaderId = glCreateShader(shaderType);
    const char *cstr = shaderCode.c_str();
    glShaderSource(shaderId, 1, &cstr, nullptr);
    glCompileShader(shaderId);

    // Check if successful
    int bSuccess = false;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &bSuccess);
    
    if (!bSuccess) {
        printe("{} failed to compile", path);
    }

    // Get the log
    std::vector<char> log(COMPILE_LOG_SIZE);
    GLsizei logLen = 0;
    glGetShaderInfoLog(shaderId, COMPILE_LOG_SIZE, &logLen, log.data());

    if (logLen != 0) {
        if (bSuccess) {
            printw("{} compiled with warnings:", path);
            printw("{}", std::string_view(log.data(), logLen));
        } else {
            printe("{}", std::string_view(log.data(), logLen));
        }
    }

    if (!bSuccess) {
        glDeleteShader(shaderId);
        return 0;
    }
    
    return shaderId;
}

bool BaseShader::linkProgram() {
    AFW_ASSERT(m_nProgId);
    AFW_ASSERT(m_nVertShaderId);
    AFW_ASSERT(m_nFragShaderId);

    glAttachShader(m_nProgId, m_nVertShaderId);
    glAttachShader(m_nProgId, m_nFragShaderId);
    glLinkProgram(m_nProgId);

    // Check if successful
    int bSuccess = false;
    glGetProgramiv(m_nProgId, GL_LINK_STATUS, &bSuccess);

    if (!bSuccess) {
        printe("{} failed to link", getTitle());
    }

    // Get the log
    std::vector<char> log(COMPILE_LOG_SIZE);
    GLsizei logLen = 0;
    glGetProgramInfoLog(m_nProgId, COMPILE_LOG_SIZE, &logLen, log.data());

    if (logLen != 0) {
        if (bSuccess) {
            printw("{} linked with warnings:", getTitle());
            printw("{}", std::string_view(log.data(), logLen));
        } else {
            printe("{}", std::string_view(log.data(), logLen));
        }
    }

    if (!bSuccess) {
        return false;
    }

    glDeleteShader(m_nVertShaderId);
    glDeleteShader(m_nFragShaderId);
    m_nVertShaderId = 0;
    m_nFragShaderId = 0;

    // Load uniforms
    for (auto i : m_UniformList)
        i->loadLocation();

    return true;
}

std::forward_list<BaseShader *> &BaseShader::getUnregItems() {
    static std::forward_list<BaseShader *> list;
    return list;
}

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
        printw("{}: uniform {} not found (may have been optimized out)", m_pShader->getTitle(),
               m_UniformName);
    }
}
