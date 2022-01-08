#ifndef RAD_BOUNCER_H
#define RAD_BOUNCER_H
#include <vector>
#include "types.h"

namespace rad {

class RadSimImpl;

class Bouncer {
public:
    //! Maximum distance from any point in world to a point on a sky face.
    static constexpr float SKY_RAY_LENGTH = 8192.f;

    inline Bouncer(RadSimImpl &radSim)
        : m_RadSim(radSim) {}

    //! Initializes the bouncer
    void setup(int bounceCount);

    //! Adds all lighting
    void addLighting();

    //! Adds light to a patch
    void addPatchLight(PatchIndex patch, const glm::vec3 &light);

    //! Bounces light around
    void bounceLight();

private:
    RadSimImpl &m_RadSim;
    int m_iBounceCount = 0;
    PatchIndex m_uPatchCount = 0;
    std::vector<glm::vec3> m_PatchBounce;

    //! Sum of all light for each patch
    std::vector<glm::vec3> m_PatchSum;

    //! Returns reference to color of patch in specified bounce.
    //! Bounce 0 is initial color.
    inline AFW_FORCE_INLINE glm::vec3 &getPatchBounce(PatchIndex patch, int bounce) {
        AFW_ASSERT(patch < m_uPatchCount && bounce <= m_iBounceCount);
        return m_PatchBounce[m_uPatchCount * (size_t)bounce + patch];
    }

    void addEnvLighting();
    void addTexLights();

    void addDirectSunlight(PatchRef &patch, const glm::vec3 vSunDir, const glm::vec3 &vSunColor);
    void addDiffuseSkylight(PatchRef &patch, const glm::vec3 &vSunColor);

    template <bool secondPass>
    void receiveLight(int bounce, PatchIndex i);
    void receiveLightFromOther(int bounce, PatchIndex i);
};

} // namespace rad 

#endif
