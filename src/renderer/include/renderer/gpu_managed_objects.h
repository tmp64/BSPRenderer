#ifndef RENDERER_GPU_MANAGED_OBJECTS_H
#define RENDERER_GPU_MANAGED_OBJECTS_H
#include <list>
#include <string>
#include <string_view>
#include <appfw/utils.h>
#include <renderer/raii.h>

class GPUResourceRecord;

namespace detail::gpu_resource {

struct BufferListGetter {
    static std::list<GPUResourceRecord> &getList();
    static void addUsedVRAM(int64_t size);
};

struct TextureListGetter {
    static std::list<GPUResourceRecord> &getList();
    static void addUsedVRAM(int64_t size);
};

struct RenderbufferListGetter {
    static std::list<GPUResourceRecord> &getList();
    static void addUsedVRAM(int64_t size);
};

//! Returns the size of an image in bytes.
int64_t getImageSize(GLint internalformat, GLsizei width, GLsizei height);

}

//! A metadata record for a GPU resource. They are stored in linked lists in GPUResourceManagers.
//! Resources only keep iterators to them.
class GPUResourceRecord {
public:
    //! Returns the name of the resource.
    inline const std::string &getName() { return m_Name; }

    //! Sets the record name.
    inline void setName(std::string_view name) { m_Name = name; }

    //! Returns how much VRAM is used by the resource.
    inline int64_t getVRAM() { return m_iVRAMUsage; }

    //! Sets the VRAM usage of this resource to a new value.
    template <typename T>
    inline void updateVRAMUsage(int64_t newSize) {
        AFW_ASSERT(newSize >= 0);
        int64_t delta = newSize - m_iVRAMUsage;
        m_iVRAMUsage = newSize;
        T::addUsedVRAM(delta);
    }

private:
    std::string m_Name;
    int64_t m_iVRAMUsage = 0;
};

//! Base class for managed GPU resources.
class GPUResource : public appfw::NoCopy {
protected:
    inline GPUResource() = default;
    inline ~GPUResource() { AFW_ASSERT(!m_bIsAllocated); }

    inline GPUResource(GPUResource &&other) noexcept {
        if (other.m_bIsAllocated) {
            m_Record = std::move(other.m_Record);
            m_bIsAllocated = true;
        } else {
            m_bIsAllocated = false;
        }

        other.m_bIsAllocated = false;
    }

    inline GPUResource &operator=(GPUResource &&other) noexcept {
        if (this != &other) {
            AFW_ASSERT(!m_bIsAllocated);

            if (other.m_bIsAllocated) {
                m_Record = std::move(other.m_Record);
                m_bIsAllocated = true;
            }

            other.m_bIsAllocated = false;
        }

        return *this;
    }

    //! Returns whether the record is allocated for the resource.
    inline bool isRecordAllocated() { return m_bIsAllocated; }

    //! Allocates a record for the resource.
    //! Must be called before resource is actually created.
    template <typename T>
    inline void allocateRecord(std::string_view name) {
        AFW_ASSERT(!m_bIsAllocated);
        m_Record = T::getList().emplace(T::getList().end());
        m_Record->setName(name);
        m_bIsAllocated = true;
    }

    //! Removes the record of the resource.
    //! Must be called after resource is actually freed.
    template <typename T>
    inline void freeRecord() {
        AFW_ASSERT(m_bIsAllocated);
        m_bIsAllocated = false;
        T::getList().erase(m_Record);
    }

    //! Sets the VRAM usage of this resource to a new value.
    template <typename T>
    inline void updateVRAMUsageInRecord(int64_t newSize) {
        AFW_ASSERT(m_bIsAllocated);
        m_Record->updateVRAMUsage<T>(newSize);
    }

private:
    std::list<GPUResourceRecord>::iterator m_Record;
    bool m_bIsAllocated = false;
};

//! Base class for resources the use GLRaiiWrapper.
//! @param  T       An instance of GLRaiiWrapper
//! @param  List    List getter
template <typename T, typename List>
class GPURaiiResource : public GPUResource {
public:
    inline GPURaiiResource() = default;
    inline GPURaiiResource(GPURaiiResource &&other) noexcept
        : GPUResource(std::move(other)) {
        m_Resource = std::move(other.m_Resource);
    }

    inline ~GPURaiiResource() { destroy(); }

    inline GPURaiiResource &operator=(GPURaiiResource &&other) noexcept {
        destroy();

        if (this != &other) {
            GPUResource::operator=(std::move(other));
            m_Resource = std::move(other.m_Resource);
        }

        return *this;
    }

    //! Returns the OpenGL id
    inline GLuint getId() const { return m_Resource.id(); }

    //! Creates the resource
    inline void create(std::string_view name) {
        destroy();
        allocateRecord<List>(name);
        m_Resource.create();
    }

    //! Destroys the resource;
    inline void destroy() noexcept {
        AFW_ASSERT((m_Resource.id() != 0) == isRecordAllocated());

        if (m_Resource.id() != 0) {
            updateVRAMUsage(0);
            freeRecord<List>();
            m_Resource.destroy();
        }
    }

protected:
    T m_Resource;

    //! Sets the VRAM usage of this resource to a new value.
    inline void updateVRAMUsage(int64_t newSize) { updateVRAMUsageInRecord<List>(newSize); }
};

//! A managed OpenGL buffer.
class GPUBuffer : public GPURaiiResource<GLBuffer, detail::gpu_resource::BufferListGetter> {
public:
    using GPURaiiResource::GPURaiiResource;

    inline void unbind(GLenum target) { glBindBuffer(target, 0); }

    inline void bind(GLenum target) {
        AFW_ASSERT_MSG(m_Resource, "This buffer is not created");
        glBindBuffer(target, getId());
    }

    inline void bufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
        AFW_ASSERT_MSG(m_Resource, "This buffer is not created");
#if AFW_DEBUG_BUILD
        GLint curBuf;
        glGetIntegerv(bindingForTarget(target), &curBuf);
        AFW_ASSERT_MSG((GLuint)curBuf == getId(), "This buffer is not bound");
#endif
        glBufferData(target, size, data, usage);
        updateVRAMUsage(size);
    }

    inline void bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
        AFW_ASSERT_MSG(m_Resource, "This buffer is not created");
#if AFW_DEBUG_BUILD
        GLint curBuf;
        glGetIntegerv(bindingForTarget(target), &curBuf);
        AFW_ASSERT_MSG((GLuint)curBuf == getId(), "This buffer is not bound");
#endif
        glBufferSubData(target, offset, size, data);
    }

private:
    [[maybe_unused]] static inline GLenum bindingForTarget(GLenum target) {
        switch (target) {
        case GL_ARRAY_BUFFER:
            return GL_ARRAY_BUFFER_BINDING;
        case GL_ELEMENT_ARRAY_BUFFER:
            return GL_ELEMENT_ARRAY_BUFFER_BINDING;
        case GL_PIXEL_PACK_BUFFER:
            return GL_PIXEL_PACK_BUFFER_BINDING;
        case GL_PIXEL_UNPACK_BUFFER:
            return GL_PIXEL_UNPACK_BUFFER_BINDING;
        case GL_TEXTURE_BUFFER:
            return GL_TEXTURE_BINDING_BUFFER;
        case GL_TRANSFORM_FEEDBACK_BUFFER:
            return GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
        case GL_UNIFORM_BUFFER:
            return GL_UNIFORM_BUFFER_BINDING;
        default:
            AFW_ASSERT_MSG(false, "Unknown glBindBuffer target");
            return 0;
        }
    }
};

//! A managed OpenGL texture.
class GPUTexture : public GPURaiiResource<GLTexture, detail::gpu_resource::TextureListGetter> {
public:
    using GPURaiiResource::GPURaiiResource;

    void texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
        GLint border, GLenum format, GLenum type, const void *data) {
        glTexImage2D(target, level, internalformat, width, height, border, format, type, data);

        if (level == 0) {
            updateVRAMUsage(detail::gpu_resource::getImageSize(internalformat, width, height));
        }
    }
};

//! A managed OpenGL renderbuffer.
class GPURenderbuffer
    : public GPURaiiResource<GLRenderbuffer, detail::gpu_resource::RenderbufferListGetter> {
public:
    using GPURaiiResource::GPURaiiResource;

    void renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
        glRenderbufferStorage(target, internalformat, width, height);
        updateVRAMUsage(detail::gpu_resource::getImageSize(internalformat, width, height));
    }
};

#endif
