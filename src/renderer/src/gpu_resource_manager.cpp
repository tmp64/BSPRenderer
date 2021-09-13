#include <gui_app_base/imgui_controls.h>
#include <renderer/gpu_resource_manager.h>

ConVar<bool> gpu_ui("gpu_ui", true, "Show GPU resource manager UI");

static inline double toMB(int64_t size) {
    return size / 1024.0 / 1024.0;
}

GPUResourceManager::GPUResourceManager() {
    setTickEnabled(true);
}

GPUResourceManager::~GPUResourceManager() {
    // Check for memory leaks
    bool hasLeaks = false;

    if (!m_BufferList.empty()) {
        hasLeaks = true;
        printw("GPUResourceManager: Some buffers ({}, {:.2f} MiB) have leaked", m_BufferList.size(),
               toMB(m_iBufferVRAM));
        dumpList(m_BufferList, false);
    }

    if (!m_TextureList.empty()) {
        hasLeaks = true;
        printw("GPUResourceManager: Some textures ({}, {:.3f} MiB) have leaked",
               m_TextureList.size(), toMB(m_iTextureVRAM));
        dumpList(m_TextureList, false);
    }

    if (!m_RenderbufferList.empty()) {
        hasLeaks = true;
        printw("GPUResourceManager: Some renderbuffers ({}, {:.3f} MiB) have leaked",
               m_RenderbufferList.size(), toMB(m_iRenderbufferVRAM));
        dumpList(m_RenderbufferList, false);
    }

    if (hasLeaks) {
        printw("*** VRAM LEAKS DETECTED ***");
    }

    AFW_ASSERT(!hasLeaks && m_iTotalVRAMUsage == 0);
}

void GPUResourceManager::tick() {
    if (gpu_ui.getValue()) {
        if (ImGuiBeginCvar("GPU Resources", gpu_ui)) {
            ImGui::Text("Total used VRAM: %.3f MiB", toMB(m_iTotalVRAMUsage));
            ImGui::Text("Buffers: %u, %.3f MiB", (unsigned)m_BufferList.size(),
                        toMB(m_iBufferVRAM));
            ImGui::Text("Textures: %u, %.3f MiB", (unsigned)m_TextureList.size(),
                        toMB(m_iTextureVRAM));
            ImGui::Text("Renderbuffers: %u, %.3f MiB", (unsigned)m_RenderbufferList.size(),
                        toMB(m_iRenderbufferVRAM));
        }

        ImGui::End();
    }
}

void GPUResourceManager::dumpList(std::list<GPUResourceRecord> &list, bool limitless) {
    auto it = list.begin();
    size_t count = 0;

    for (; (limitless || count < MAX_DUMP_LIST_ITEMS) && it != list.end(); ++it, ++count) {
        printi("- {}, {:.3f} MiB", it->getName(), toMB(it->getVRAM()));
    }

    if (count != list.size()) {
        printi("   ... {} more", list.size() - count);
    }
}
