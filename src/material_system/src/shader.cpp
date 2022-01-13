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

void Shader::createShaderInstances() {
    for (unsigned i = 0; i < MAX_SHADER_TYPE_COUNT; i++) {
        if (m_uShaderTypes & (1 << i)) {
            std::unique_ptr<Shader> shaderInfo = createShaderInfoInstance(i);
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
}
