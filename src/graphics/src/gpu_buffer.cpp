#include <graphics/gpu_buffer.h>

GPUBuffer::~GPUBuffer() {
    destroy();
}

void GPUBuffer::create(GLenum target, std::string_view name) {
    m_Target = target;
    m_Buf.create();
    m_Target = target;
    m_Name = name;
}

void GPUBuffer::destroy() {
    if (m_Buf) {
        m_Buf.destroy();
        updateMemUsage(0);
    }
}

void GPUBuffer::init(GLsizeiptr size, const void *data, GLenum usage) {
    bind();
    glBufferData(m_Target, size, data, usage);
    updateMemUsage(size);
}

void GPUBuffer::update(GLintptr offset, GLsizeiptr size, const void *data) {
    bind();
    glBufferSubData(m_Target, offset, size, data);
}

void GPUBuffer::updateMemUsage(int64_t usage) {
    int64_t delta = usage - m_MemUsage.get();
    m_MemUsage.set(usage);
    m_siGlobalMemUsage += delta;
}
