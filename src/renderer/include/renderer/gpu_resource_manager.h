#ifndef RENDERER_GPU_RESOURCE_MANAGER_H
#define RENDERER_GPU_RESOURCE_MANAGER_H
#include <app_base/app_component.h>
#include <renderer/gpu_managed_objects.h>

class GPUResourceManager : public AppComponentBase<GPUResourceManager> {
public:
    GPUResourceManager();
    ~GPUResourceManager();

    void tick() override;

private:
    static constexpr size_t MAX_DUMP_LIST_ITEMS = 16;

    int64_t m_iTotalVRAMUsage = 0;

    std::list<GPUResourceRecord> m_BufferList;
    int64_t m_iBufferVRAM = 0;

    std::list<GPUResourceRecord> m_TextureList;
    int64_t m_iTextureVRAM = 0;

    std::list<GPUResourceRecord> m_RenderbufferList;
    int64_t m_iRenderbufferVRAM = 0;

    //! Prints contents of the list into the console
    //! @param  limitless   If false, prints at most MAX_DUMP_LIST_ITEMS items 
    void dumpList(std::list<GPUResourceRecord> &list, bool limitless);

    friend struct detail::gpu_resource::BufferListGetter;
    friend struct detail::gpu_resource::TextureListGetter;
    friend struct detail::gpu_resource::RenderbufferListGetter;
};

#endif
