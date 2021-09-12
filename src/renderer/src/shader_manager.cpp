#include <appfw/appfw.h>
#include <renderer/base_shader.h>
#include <renderer/shader_manager.h>

ConCommand r_reloadshaders("r_reloadshaders", "Reloads all shaders", [](auto &) { ShaderManager::get().reloadShaders(); });

ShaderManager::ShaderManager() {
    // Add default definitions
    getVertDefinitions().addDef("IFACE_VF", "out");
    getFragDefinitions().addDef("IFACE_VF", "in");

    registerAllAvailableItems();
    reloadShaders();
}

ShaderManager::~ShaderManager() {
    destroyAllShaders();
}

void ShaderManager::registerAllAvailableItems() {
    auto &list = BaseShader::getUnregItems();
    size_t count = 0;

    auto it = list.begin();
    while (it != list.end()) {
        BaseShader *pItem = *it;
        m_ShaderList.push_back(pItem);

        list.pop_front();
        it = list.begin();
        count++;
    }

    m_ShaderList.shrink_to_fit();
}

bool ShaderManager::reloadShaders() {
    printi("Reloading all shaders");

    destroyAllShaders();

    size_t success = 0;
    size_t failed = 0;
    bool result = true;

    for (BaseShader *pShader : m_ShaderList) {
        if (pShader->reload()) {
            success++;
        } else {
            failed++;
            result = false;
        }
    }

    printi("Reloaded {} shader(s), {} failed (total {})", success, failed, m_ShaderList.size());

    if (!result) {
        AFW_DEBUG_BREAK();
    }

    return result;
}

void ShaderManager::destroyAllShaders() {
    for (BaseShader *pShader : m_ShaderList) {
        pShader->destroy();
    }
}
