#ifndef RAD_PATCH_DIVIDER_H
#define RAD_PATCH_DIVIDER_H
#include <appfw/utils.h>
#include <appfw/sha256.h>
#include "types.h"

namespace rad {

//! Divides surfaces into patches. Patches that don't completely fit onto the face are subdivided
//! further until a fixed minimum size. In the end the patches are transferred into the main list.
//! Can (and should) be called for multiple surface. Not thread-safe.
class PatchDivider : appfw::NoMove {
public:
    ~PatchDivider();
    
    //! Creates mini-patches for the face. Stored in an internal linked list.
    //! @returns the number of created patches.
    PatchIndex createPatches(RadSimImpl *pRadSim, Face &face, PatchIndex offset);

    //! Moves patches to the main patch list.
    //! @returns the number of created patches.
    PatchIndex transferPatches(RadSimImpl *pRadSim, appfw::SHA256 &hash);

private:
    enum class PatchPos
    {
        Inside,          //!< Patch is completely inside the face
        PartiallyInside, //!< Patch is partially inside and outside
        Outside          //!< Patch is completely outside the face
    };

    //! Basic patch data. Stored in a linked list before creating full Patch instances.
    struct MiniPatch {
        MiniPatch *pNext = nullptr;
        Face *pFace = nullptr;
        float flSize = 0;

        //! Face coords of the lower corner of the patch (not the center!)
        glm::vec2 vFaceOrigin;
    };

    MiniPatch *m_pPatches = nullptr;
    MiniPatch *m_pLastPatch = nullptr;

    //! Divides the patch into up to 4 new patches.
    //! @param  patch           Original patch
    //! @param  newFirstPatch   Will be set to new first patch
    //! @param  newLastPatch    Will be set to new last patch
    //! @param  keepPatch       If false, original patch must be removed
    //! @param  flMinPatchSize  Minimum patch size
    //! @return the number of created patches
    static PatchIndex subdividePatch(Face &face, MiniPatch *patch, MiniPatch *&newFirstPatch,
                                     MiniPatch *&newLastPatch, bool &keepPatch,
                                     float flMinPatchSize);

    //! @returns how patch is positioned on the face
    static PatchPos checkPatchPos(Face &face, MiniPatch *patch) noexcept;

    //! @returns whether the point is located in the surface polygon.
    static bool isInFace(Face &face, glm::vec2 point) noexcept;

    //! @returns whether the point is located in the patch.
    static bool pointInWithPatch(glm::vec2 point, MiniPatch *patch);
};

} // namespace rad

#endif
