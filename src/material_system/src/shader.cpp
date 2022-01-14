#include <material_system/shader.h>
#include <material_system/shader_instance.h>

Shader::Shader(unsigned type) {
    if (type == 0) {
        getPrototypeList().push_back(this);
    }
}

Shader::~Shader() {
    // All instances must be destroyed
    if constexpr (appfw::isDebugBuild()) {
        for (unsigned i = 0; i < MAX_SHADER_TYPE_COUNT; i++) {
            AFW_ASSERT(!m_pInstances[i]);
        }
    }
}

std::list<Shader *> &Shader::getPrototypeList() {
    static std::list<Shader *> list;
    return list;
}

void Shader::addUniform(ShaderUniform &uniform, const char *name) {
    uniform.setName(name);
    m_Uniforms.push_back(&uniform);
}

void Shader::addUniform(UniformBlock &uniform, const char *name, GLuint bindingPoint) {
    uniform.setName(name);
    uniform.setBindingPoint(bindingPoint);
    m_UniformBlocks.push_back(&uniform);
}

void Shader::onShaderCompiled() {}

void Shader::createShaderInstances() {
    for (unsigned i = 0; i < MAX_SHADER_TYPE_COUNT; i++) {
        if (m_uShaderTypes & (1u << i)) {
            std::unique_ptr<Shader> shaderInfo = createShaderInfoInstance(1u << i);
            auto shader = std::make_unique<ShaderInstance>(shaderInfo);

            if (shader->compile()) {
                m_pInstances[i] = std::move(shader);
            } else {
                printe("Shader {} type {} failed to compile", m_Title, SHADER_TYPE_NAMES[i]);
            }
        }
    }
}

void Shader::freeShaderInstances() {
    for (unsigned i = 0; i < MAX_SHADER_TYPE_COUNT; i++) {
        m_pInstances[i] = nullptr;
    }
}

void Shader::loadUniformLocations(GLuint progId) {
    for (ShaderUniform *uniform : m_Uniforms) {
        uniform->loadLocation(progId);
    }

    for (UniformBlock *uniform : m_UniformBlocks) {
        uniform->load(progId);
    }
}

void Shader::UniformBlock::load(GLuint progId) {
    m_nIndex = glGetUniformBlockIndex(progId, m_pszUniformName);

    if (m_nIndex == GL_INVALID_INDEX) {
        printe("Uniform block {} not found", m_pszUniformName);
    }

    glUniformBlockBinding(progId, m_nIndex, m_nBindingPoint);
}
