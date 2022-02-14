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

    //! Minimum brightness for light to be considered to light up the surface.
    static constexpr float GAMMA_INTENSITY_THRESHOLD = 4.0f / 255.0f;

    Bouncer(RadSimImpl &radSim);
    void setup(int lightstyle, int bounceCount);
    void addSunLight();
    void addSkyLight();
    void addEntLight(const EntLight &el);
    void addTexLight(int faceIdx);
    void calcLight();

private:
    RadSimImpl &m_RadSim;
    float m_flLinearThreshold = 0;
    PatchIndex m_uPatchCount = 0;
    int m_iLightstyle = -1;
    int m_iBounceCount = 0;
    size_t m_uWorkerCount = 0;
    std::vector<glm::vec3> m_WorkerData;      //!< Buffer for the workers
    std::vector<glm::vec3> m_Texlights;       //!< Texlights. Added in a separate radiosity pass to direct lighting.
    std::vector<glm::vec3> m_TotalPatchLight; //!< Sum of m_PatchBounce for all bounces as well as initial direct lighting as bounce 0.
    std::vector<glm::vec3> m_PatchBounce;     //!< Patch colors for each bounce.

    //! Returns reference to color of patch in specified bounce.
    //! Bounce 0 is initial color.
    inline AFW_FORCE_INLINE glm::vec3 &getPatchBounce(PatchIndex patch, int bounce) {
        AFW_ASSERT(patch < m_uPatchCount && bounce <= m_iBounceCount);
        return m_PatchBounce[m_uPatchCount * (size_t)bounce + patch];
    }

    inline AFW_FORCE_INLINE glm::vec3 &getWorkerData(PatchIndex patch, size_t worker) {
        return m_WorkerData[m_uPatchCount * worker + patch];
    }

    void addPointLightToFace(Face &face, const EntLight &el);

    void radiateTexLights();    //!< Radiosity pass for texlight direct lighting.
    void bounceLight();         //!< Main radiosity pass with multiple bounces.
    void calcTotalLight();      //!< Fills m_TotalPatchLight with sum of all bounces.
    void assignLightStyles();   //!< Assigns lightstyle to faces that received enough light.
};

} // namespace rad 

#endif
