#ifndef RENDERER_SHADER_MANAGER_H
#define RENDERER_SHADER_MANAGER_H
#include <vector>

class BaseShader;

class ShaderManager {
public:
    /**
     * Returns instance of shader manager.
     */
    static ShaderManager &get();

    /**
     * Initializes shader manager.
     * Calls registerAllAvailableItems.
     */
    void init();

    /**
     * Deletes all shaders.
     */
    void shutdown();

    /**
     * Adds all unregistered shaders to this system.
     */
    void registerAllAvailableItems();

    /**
     * Forces reload and recompilation of all shaders.
     * On error prints everything to the console.
     * @return Success status.
     */
    bool reloadShaders();

private:
    std::vector<BaseShader *> m_ShaderList;

    void destroyAllShaders();
};

#endif
