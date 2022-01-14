#ifndef MATERIAL_SYSTEM_SHADER_H
#define MATERIAL_SYSTEM_SHADER_H
#include <appfw/utils.h>
#include <material_system/shader_types.h>
#include <material_system/shader_definitions.h>
#include <material_system/shader_uniform.h>

class MaterialSystem;
class ShaderInstance;

class Shader : appfw::NoMove {
public:
    class UniformBlock;

    //! @param  type    Type of the shader being created or 0 if it's the prototype
    Shader(unsigned type);
    virtual ~Shader();

    //! @param  idx     Shader type index
    //! @returns an instance of the shader of specified type.
    inline ShaderInstance *getShaderInstance(unsigned idx) { return m_pInstances[idx].get(); }

    //! @returns the list of shader prototypes.
    static std::list<Shader *> &getPrototypeList();

protected:
    //! Sets the shader title.
    inline void setTitle(std::string_view title) { m_Title = title; }

    //! Sets the virtual path to the vertex shader.
    inline void setVert(std::string_view vertexPath) { m_VertexFilePath = vertexPath; }

    //! Sets the virtual path to the fragment shader.
    inline void setFrag(std::string_view fragmentPath) { m_FragmentFilePath = fragmentPath; }

    //! Sets the shader types that this shader supports.
    inline void setTypes(unsigned types) { m_uShaderTypes = types; }

    /*//! Adds a shared definition to shader of specified types.
    template <typename T>
    void addSharedDef(unsigned type, const T &value) {
        forEachShaderTypeDef(type, [&](ShaderProgramDefinitions &d) { d.addSharedDef(value); });
    }

    //! Adds a vertex definition to shader of specified types.
    template <typename T>
    void addVertexDef(unsigned type, const T &value) {
        forEachShaderTypeDef(type, [&](ShaderProgramDefinitions &d) { d.addVertexDef(value); });
    }

    //! Adds a fragment definition to shader of specified types.
    template <typename T>
    void addFragmentDef(unsigned type, const T &value) {
        forEachShaderTypeDef(type, [&](ShaderProgramDefinitions &d) { d.addFragmentDef(value); });
    }*/

    //! Adds a shared definition to the shader.
    template <typename T>
    void addSharedDef(std::string_view key, const T &value) {
        m_Defs.addSharedDef(key, value);
    }

    //! Adds a vertex definition to the shader.
    template <typename T>
    void addVertexDef(std::string_view key, const T &value) {
        m_Defs.addVertexDef(key, value);
    }

    //! Adds a fragment definition to the shader.
    template <typename T>
    void addFragmentDef(std::string_view key, const T &value) {
        m_Defs.addFragmentDef(key, value);
    }

    //! Adds a uniform to the list.
    //! @param  name    Name of the uniform, must be a constant string
    void addUniform(ShaderUniform &uniform, const char *name);

    //! Adds a uniform block to the list.
    //! @param  name    Name of the uniform, must be a constant string
    void addUniform(UniformBlock &uniform, const char *name, GLuint bindingPoint);

    //! Called when the shader was compiled. It will be enabled in this call.
    virtual void onShaderCompiled();

private:
    // Prototype information
    unsigned m_uShaderTypes = 0;

    // Shader information (set in constructor)
    std::string_view m_Title;
    std::string_view m_VertexFilePath;
    std::string_view m_FragmentFilePath;
    std::vector<ShaderUniform *> m_Uniforms;
    std::vector<UniformBlock *> m_UniformBlocks;
    ShaderProgramDefinitions m_Defs;

    // Shader instances (actual OpenGL shaders)
    std::unique_ptr<ShaderInstance> m_pInstances[MAX_SHADER_TYPE_COUNT];

    /*template <typename F>
    void forEachShaderTypeDef(unsigned type, F func) {
        for (unsigned i = 0; i < MAX_SHADER_TYPE_COUNT; i++) {
            if (type & (1 << i)) {
                func(m_Defs[i]);
            }
        }
    }*/

    //! Creates an instance of shader info of specified type.
    virtual std::unique_ptr<Shader> createShaderInfoInstance(unsigned typeIdx) = 0;

    //! Creates necessary shader instances.
    void createShaderInstances();

    //! Frees all shader instances.
    void freeShaderInstances();

    void loadUniformLocations(GLuint progId);

    friend class MaterialSystem;
    friend class ShaderInstance;

public:
    template <typename T>
    class Var : public ShaderUniform {
        inline Var() { static_assert(appfw::FalseT<T>::value, "Unknown uniform type"); }
    };

    template <>
    class Var<int> : public ShaderUniform {
    public:
        inline void set(int val) { glUniform1i(m_nLocation, val); }
    };

    template <>
    class Var<float> : public ShaderUniform {
    public:
        inline void set(float val) { glUniform1f(m_nLocation, val); }
    };

    template <>
    class Var<glm::vec3> : public ShaderUniform {
    public:
        inline void set(const glm::vec3 &v) { glUniform3f(m_nLocation, v.x, v.y, v.z); }
    };

    template <>
    class Var<glm::vec4> : public ShaderUniform {
    public:
        inline void set(const glm::vec4 &v) { glUniform4f(m_nLocation, v.x, v.y, v.z, v.w); }
    };

    template <>
    class Var<glm::mat4> : public ShaderUniform {
    public:
        inline void set(const glm::mat4 &v) {
            glUniformMatrix4fv(m_nLocation, 1, false, glm::value_ptr(v));
        }
    };

    class TextureVar : public Var<int> {};

    class UniformBlock : appfw::NoCopy {
    public:
        inline void setName(const char *name) { m_pszUniformName = name; }
        inline void setBindingPoint(GLuint bindingPoint) { m_nBindingPoint = bindingPoint; }
        void load(GLuint progId);

    private:
        const char *m_pszUniformName;
        GLuint m_nIndex = 0;
        GLuint m_nBindingPoint = 0;

        friend class BaseShader;
    };
};

template <typename T>
class ShaderT : public Shader {
public:
    using BaseClass = ShaderT<T>;

    inline ShaderT(unsigned typeIdx)
        : Shader(typeIdx) {}

private:
    inline std::unique_ptr<Shader> createShaderInfoInstance(unsigned typeIdx) override {
        return std::make_unique<T>(typeIdx);
    }
};

#endif
