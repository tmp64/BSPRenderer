#include <stb_include.h>
#include <material_system/shader_instance.h>
#include <material_system/shader.h>
#include <material_system/material_system.h>

ShaderInstance::ShaderInstance(std::unique_ptr<Shader> &shader) {
    m_pShader = std::move(shader);
}

void ShaderInstance::enable() {
    glUseProgram(m_Prog.getId());
    m_spCurrentInstance = this;
}

void ShaderInstance::disable() {
    glUseProgram(0);
    m_spCurrentInstance = nullptr;
}

bool ShaderInstance::compile() {
    ShaderStage vertexShader;
    vertexShader.createVertexShader();
    if (!compileShader(vertexShader, m_pShader->m_VertexFilePath,
                       MaterialSystem::get().getGlobalVertexShaderDefs(),
                       m_pShader->m_Defs.toStringVertex())) {
        return false;
    }

    ShaderStage fragmentShader;
    vertexShader.createFragmentShader();
    if (!compileShader(fragmentShader, m_pShader->m_FragmentFilePath,
                       MaterialSystem::get().getGlobalFragmentShaderDefs(),
                       m_pShader->m_Defs.toStringFragment())) {
        return false;
    }

    m_Prog.create();
    m_Prog.attachShader(vertexShader);
    m_Prog.attachShader(fragmentShader);

    std::string errorLog;

    if (!m_Prog.link(errorLog)) {
        printe("{}: link failed", m_pShader->m_Title);
        printe("{}", errorLog);
        return false;
    } else if (!errorLog.empty()) {
        printw("{}: linked with warnings", m_pShader->m_Title);
        printw("{}", errorLog);
    }

    return true;
}

bool ShaderInstance::compileShader(ShaderStage &stage, std::string_view vpath,
                                   std::string_view globalDefines, std::string_view defines) {
    // Resolve path
    fs::path fullPath = getFileSystem().findExistingFile(vpath, std::nothrow);

    if (fullPath.empty()) {
        printe("Shader file {} not found", vpath);
        return false;
    }

    // Read source file
    std::vector<char> rawData;
    std::ifstream file(fullPath, std::ifstream::binary);
    appfw::readFileContents(file, rawData);
    rawData.push_back('\0');

    // Handle #include
    char includeError[256];
    std::unique_ptr<char, decltype(&std::free)> processedFile(
        stb_include_string(rawData.data(), nullptr, (char *)"assets:shaders", nullptr,
                           includeError),
        &free);

    if (!processedFile) {
        printe("{}: include processing failed", vpath);
        printe("{}", includeError);
        return "";
    }

    // Assemble source header
    std::string srcHeader;
    srcHeader += "#version 330 core\n";
    srcHeader += globalDefines;
    srcHeader += defines;
    srcHeader += "#line 1\n";
    srcHeader += processedFile.get();

    // Compile the shader
    if (!stage.compile(srcHeader)) {
        printe("{}: compilation failed", vpath);
        printe("{}", stage.getCompileLog());
        return false;
    } else if (!stage.getCompileLog().empty()) {
        printw("{}: compiled with warnings", vpath);
        printw("{}", stage.getCompileLog());
    }

    return true;
}
