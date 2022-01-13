#include <material_system/material_system.h>
#include <material_system/shader.h>
#include <material_system/shader_instance.h>

MaterialSystem::MaterialSystem() {
    setTickEnabled(true);
    reloadShaders();
}

MaterialSystem::~MaterialSystem() {
    unloadShaders();
}

void MaterialSystem::tick() {}

void MaterialSystem::reloadShaders() {
    unloadShaders();

    auto &prototypes = Shader::getPrototypeList();

    for (Shader *pShader : prototypes) {
        pShader->createShaderInstances();
    }
}

void MaterialSystem::unloadShaders() {
    // Make sure no dangling pointers are stored
    ShaderInstance::disable();

    auto &prototypes = Shader::getPrototypeList();

    for (Shader *pShader : prototypes) {
        pShader->freeShaderInstances();
    }
}
