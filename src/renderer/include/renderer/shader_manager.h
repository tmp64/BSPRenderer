#ifndef RENDERER_SHADER_MANAGER_H
#define RENDERER_SHADER_MANAGER_H
#include <vector>
#include <app_base/app_component.h>
#include <renderer/shader_definitions.h>

class BaseShader;

class ShaderManager : public AppComponentBase<ShaderManager> {
public:
    ShaderManager();
    ~ShaderManager();

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

    //! Returns the list of shader definitions applied to all shaders
    inline ShaderDefinitions &getGlobalDefinitions() { return m_GlobalDefs; }

    //! Returns the list of shader definitions applied to all vertex shaders
    inline ShaderDefinitions &getVertDefinitions() { return m_VertDefs; }

    //! Returns the list of shader definitions applied to all fragment shaders
    inline ShaderDefinitions &getFragDefinitions() { return m_FragDefs; }

private:
    std::vector<BaseShader *> m_ShaderList;
    ShaderDefinitions m_GlobalDefs;
    ShaderDefinitions m_VertDefs;
    ShaderDefinitions m_FragDefs;

    void destroyAllShaders();
};

#endif
