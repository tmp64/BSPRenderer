#include <renderer/gpu_managed_objects.h>
#include <renderer/gpu_resource_manager.h>

std::list<GPUResourceRecord> &detail::gpu_resource::BufferListGetter::getList() {
    return GPUResourceManager::get().m_BufferList;
}

void detail::gpu_resource::BufferListGetter::addUsedVRAM(int64_t size) {
    auto &mgr = GPUResourceManager::get();
    mgr.m_iTotalVRAMUsage += size;
    mgr.m_iBufferVRAM += size;
}

std::list<GPUResourceRecord> &detail::gpu_resource::TextureListGetter::getList() {
    return GPUResourceManager::get().m_TextureList;
}

void detail::gpu_resource::TextureListGetter::addUsedVRAM(int64_t size) {
    auto &mgr = GPUResourceManager::get();
    mgr.m_iTotalVRAMUsage += size;
    mgr.m_iTextureVRAM += size;
}

std::list<GPUResourceRecord> &detail::gpu_resource::RenderbufferListGetter::getList() {
    return GPUResourceManager::get().m_RenderbufferList;
}

void detail::gpu_resource::RenderbufferListGetter::addUsedVRAM(int64_t size) {
    auto &mgr = GPUResourceManager::get();
    mgr.m_iTotalVRAMUsage += size;
    mgr.m_iRenderbufferVRAM += size;
}

int64_t detail::gpu_resource::getImageSize(GLint internalformat, GLsizei width, GLsizei height) {
    GLsizei pixelSize = 0;

    switch (internalformat) {
    case GL_RED:
    case GL_RG:
    case GL_RGB:
    case GL_RGBA:
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_STENCIL:
        AFW_ASSERT_REL_MSG(false, "Use sized format instead");
        abort();
        break;

    case GL_R8:
    case GL_R8_SNORM:
    case GL_R3_G3_B2:
    case GL_RGBA2:
    case GL_R8I:
    case GL_R8UI:
    case GL_STENCIL_INDEX8:
        pixelSize = 1;
        break;

    case GL_R16:
    case GL_R16_SNORM:
    case GL_RG8:
    case GL_RG8_SNORM:
    case GL_RGB4:
    case GL_RGB5:
    case GL_RGBA4:
    case GL_RGB5_A1:
    case GL_R16F:
    case GL_R16I:
    case GL_R16UI:
    case GL_RG8I:
    case GL_RG8UI:
    case GL_DEPTH_COMPONENT16:
        pixelSize = 2;
        break;

    case GL_RG16:
    case GL_RG16_SNORM:
    case GL_RGB8:
    case GL_RGB8_SNORM:
    case GL_RGB10:
    case GL_RGBA8:
    case GL_RGBA8_SNORM:
    case GL_RGB10_A2:
    case GL_RGB10_A2UI:
    case GL_SRGB8:
    case GL_SRGB8_ALPHA8:
    case GL_RG16F:
    case GL_R32F:
    case GL_R11F_G11F_B10F:
    case GL_RGB9_E5:
    case GL_R32I:
    case GL_R32UI:
    case GL_RG16I:
    case GL_RG16UI:
    case GL_RGB8I:
    case GL_RGB8UI:
    case GL_RGBA8I:
    case GL_RGBA8UI:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH_COMPONENT32F:
    case GL_DEPTH24_STENCIL8:
        pixelSize = 4;
        break;

    case GL_RGB12:
    case GL_RGBA12:
    case GL_RGBA16:
    case GL_RGB16F:
    case GL_RGBA16F:
    case GL_RG32F:
    case GL_RG32I:
    case GL_RG32UI:
    case GL_RGB16I:
    case GL_RGB16UI:
    case GL_RGBA16I:
    case GL_RGBA16UI:
    case GL_DEPTH32F_STENCIL8:
        pixelSize = 8; // assume the worst?
        break;

    case GL_RGB16_SNORM:
    case GL_RGB32F:
    case GL_RGB32I:
    case GL_RGB32UI:
        pixelSize = 12;
        break;

    case GL_RGBA32F:
    case GL_RGBA32I:
    case GL_RGBA32UI:
        pixelSize = 16;
        break;

    default:
        AFW_ASSERT_REL_MSG(false, "Invalid internal format");
        abort();
        break;
    }

    GLsizei rowSize = pixelSize * width;
    GLsizei alignment = std::max(pixelSize, 4);

    if (rowSize % alignment != 0) {
        rowSize += rowSize % alignment;
    }

    GLsizei size = rowSize * height;

    // Most textures have mipmaps so account for that as well.
    // With mipmaps size is 4/3 times larger (so add 1/3).
    size += (GLsizei)(size / 3.0);

    return (int64_t)size;
}
