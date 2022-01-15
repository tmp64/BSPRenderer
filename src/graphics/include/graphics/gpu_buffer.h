#ifndef GRAPHICS_GPU_BUFFER_H
#define GRAPHICS_GPU_BUFFER_H
#include <string_view>
#include <graphics/raii.h>
#include <graphics/vram_tracker.h>

class GPUBuffer {
public:
    GPUBuffer() = default;
    GPUBuffer(GPUBuffer &&other) noexcept;
    ~GPUBuffer();
    GPUBuffer &operator=(GPUBuffer &&other) noexcept;

    //! Creates an OpenGL buffer name.
    void create(GLenum target, std::string_view name);

    //! Destroys the buffer, clearing the data.
    void destroy();

    //! Binds the buffer (calls glBindBuffer);
    inline void bind() const { glBindBuffer(m_Target, m_Buf); }

    //! Unbinds the buffer (or any other buffer from the target).
    inline void unbind() const { glBindBuffer(m_Target, 0); }

    //! Creates the buffer. data can be null.
    //! Changes currently bound buffer.
    void init(GLsizeiptr size, const void *data, GLenum usage);

    //! Updates the buffer data.
    //! Changes currently bound buffer.
    void update(GLintptr offset, GLsizeiptr size, const void *data);

    //! @returns OpenGL buffer ID.
    inline GLuint getId() const { return m_Buf.id(); }

    //! @returns buffer name.
    inline const std::string &getName() const { return m_Name; }

    //! @returns VRAM usage.
    inline int64_t getMemUsage() const { return m_MemUsage.get(); }

    //! @returns VRAM used by all buffers.
    static int64_t getGlobalMemUsage() { return m_siGlobalMemUsage; }

private:
    GLenum m_Target = 0;
    GLBuffer m_Buf;
    VRAMTracker m_MemUsage;
    std::string m_Name;

    void updateMemUsage(int64_t usage);

    static inline int64_t m_siGlobalMemUsage = 0;
};

#endif
