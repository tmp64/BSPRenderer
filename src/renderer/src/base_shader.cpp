#include <fstream>
#include <stb_include.h>
#include <appfw/dbg.h>
#include <renderer/base_shader.h>
#include <renderer/shader_manager.h>

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

void BaseShader::addUniform(UniformBase &uniform, const char *name) {
    uniform.m_pszUniformName = name;
    m_UniformList.push_back(&uniform);
}

void BaseShader::createProgram() {
    AFW_ASSERT(!m_nProgId);
    m_nProgId = glCreateProgram();
}

GLuint BaseShader::compileShader(GLenum shaderType, std::string_view path) {
    std::string shaderCode = loadShaderFile(shaderType, path);

    if (shaderCode.empty()) {
        return 0;
    }

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

std::string BaseShader::loadShaderFile(GLenum shaderType, std::string_view path) {
    // Resolve path
    fs::path fullPath = getFileSystem().findExistingFile(path, std::nothrow);

    if (fullPath.empty()) {
        printe("{}: file {} not found", m_Title, path);
        return "";
    }

    // Read source file
    std::vector<char> rawData;
    std::ifstream file(fullPath, std::ifstream::binary);
    appfw::readFileContents(file, rawData);
    rawData.push_back('\0');

    // Handle #include
    char includeError[256];
    std::unique_ptr<char, decltype(&std::free)> processedFile(
        stb_include_string(rawData.data(), nullptr, "assets:shaders", nullptr, includeError),
        &free);

    if (!processedFile) {
        printe("{}: {}: include processing failed", m_Title, path);
        printe("{}", includeError);
        return "";
    }

    // Assemble source header
    std::string srcHeader;
#ifdef GLAD_OPENGL
    srcHeader += "#version 330 core\n";
#elif defined(GLAD_OPENGL_ES)
    srcHeader += "#version 300 es\n";
#else
#error
#endif
    srcHeader += ShaderManager::get().getGlobalDefinitions().toString();

    if (shaderType == GL_VERTEX_SHADER) {
        srcHeader += ShaderManager::get().getVertDefinitions().toString();
    } else if (shaderType == GL_FRAGMENT_SHADER) {
        srcHeader += ShaderManager::get().getFragDefinitions().toString();
    }

    srcHeader += getDefinitions().toString();

    if (shaderType == GL_VERTEX_SHADER) {
        srcHeader += getVertDefinitions().toString();
    } else if (shaderType == GL_FRAGMENT_SHADER) {
        srcHeader += getFragDefinitions().toString();
    }

    srcHeader += "#line 1\n";

    return srcHeader + processedFile.get();
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
    for (UniformBase *i : m_UniformList) {
        loadUniformLocation(*i);
    }

    return true;
}

void BaseShader::loadUniformLocation(UniformBase &uniform) {
    uniform.m_nLocation = glGetUniformLocation(m_nProgId, uniform.m_pszUniformName);

    if (uniform.m_nLocation == -1) {
        printw("{}: uniform {} not found (may have been optimized out)", getTitle(),
               uniform.m_pszUniformName);
    }
}

std::forward_list<BaseShader *> &BaseShader::getUnregItems() {
    static std::forward_list<BaseShader *> list;
    return list;
}
