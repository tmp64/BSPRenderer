#ifndef RAD_BOUNCER_H
#define RAD_BOUNCER_H
#include <vector>
#include <rad/types.h>

namespace rad {

class RadSim;

class Bouncer {
public:
    //! Initializes the bouncer
    void setup(RadSim *pRadSim, int bounceCount);

    //! Adds light to a patch
    void addPatchLight(PatchIndex patch, const glm::vec3 &light);

    //! Bounces light around
    void bounceLight();

private:
    RadSim *m_pRadSim = nullptr;
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

    void receiveLightFromThis(int bounce, PatchIndex i);
    void receiveLightFromOther(int bounce, PatchIndex i);
};

} // namespace rad 

#endif
