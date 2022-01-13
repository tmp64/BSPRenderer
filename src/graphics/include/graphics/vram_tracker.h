#ifndef GRAPHICS_VRAM_TRACKER_H
#define GRAPHICS_VRAM_TRACKER_H
#include <cstdint>

class VRAMTracker {
public:
    VRAMTracker() = default;
    ~VRAMTracker() { AFW_ASSERT(m_iMemUsage == 0); }
    VRAMTracker(const VRAMTracker &) = delete;
    VRAMTracker(VRAMTracker &&other) noexcept {
        m_iMemUsage = other.m_iMemUsage;
        other.m_iMemUsage = 0;
    }

    VRAMTracker &operator=(const VRAMTracker &) = delete;
    VRAMTracker &operator=(VRAMTracker &&other) noexcept {
        if (&other != this) {
            m_iMemUsage = other.m_iMemUsage;
            other.m_iMemUsage = 0;
        }

        return *this;
    }

    inline int64_t get() const { return m_iMemUsage; }
    inline void set(int64_t memUsage) { m_iMemUsage = memUsage; }

private:
    int64_t m_iMemUsage = 0;
};

#endif
