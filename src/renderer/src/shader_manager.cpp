#include <appfw/services.h>
#include <renderer/base_shader.h>
#include <renderer/shader_manager.h>

ConCommand r_reloadshaders("r_reloadshaders", "Reloads all shaders", [](auto &) { ShaderManager::get().reloadShaders(); });

ShaderManager &ShaderManager::get() {
    static ShaderManager singleton;
    return singleton;
}

void ShaderManager::init() {
    registerAllAvailableItems();
    reloadShaders();
}

void ShaderManager::shutdown() { destroyAllShaders(); }

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
    logInfo("Reloading all shaders");

    destroyAllShaders();

    size_t success = 0;
    size_t failed = 0;
    bool result = true;

    for (BaseShader *pShader : m_ShaderList) {
        try {
            pShader->create();
            success++;
        } catch (const std::exception &e) {
            logError("When creating shader '{}':", pShader->getShaderTitle());
            logError("    Error: {}", e.what());
            logError("");

            result = false;
            failed++;
        }
    }

    logInfo("Reloaded {} shader(s), {} failed (total {})", success, failed, m_ShaderList.size());
    return result;
}

void ShaderManager::destroyAllShaders() {
    for (BaseShader *pShader : m_ShaderList) {
        pShader->destroy();
    }
}
