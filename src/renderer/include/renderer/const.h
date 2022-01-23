#ifndef RENDERER_CONST_H
#define RENDERER_CONST_H

//! Maximum number of entities visible in one frame
constexpr unsigned MAX_VISIBLE_ENTS = 512;

//! Maximum number of lightstyles on a map. Byte limit.
constexpr int MAX_LIGHTSTYLES = 256;

enum RenderMode : int {
    kRenderNormal,       // src
    kRenderTransColor,   // c*a+dest*(1-a)
    kRenderTransTexture, // src*a+dest*(1-a)
    kRenderGlow,         // src*a+dest -- No Z buffer checks
    kRenderTransAlpha,   // src*srca+dest*(1-srca)
    kRenderTransAdd,     // src*a+dest
};

enum RenderFx : int {
    kRenderFxNone = 0,
    kRenderFxPulseSlow,
    kRenderFxPulseFast,
    kRenderFxPulseSlowWide,
    kRenderFxPulseFastWide,
    kRenderFxFadeSlow,
    kRenderFxFadeFast,
    kRenderFxSolidSlow,
    kRenderFxSolidFast,
    kRenderFxStrobeSlow,
    kRenderFxStrobeFast,
    kRenderFxStrobeFaster,
    kRenderFxFlickerSlow,
    kRenderFxFlickerFast,
    kRenderFxNoDissipation,
    kRenderFxDistort,         // Distort/scale/translate flicker
    kRenderFxHologram,        // kRenderFxDistort + distance fade
    kRenderFxDeadPlayer,      // kRenderAmt is the player index
    kRenderFxExplode,         // Scale up really big!
    kRenderFxGlowShell,       // Glowing Shell
    kRenderFxClampMinScale,   // Keep this sprite from getting very small (SPRITES only!)
    kRenderFxLightMultiplier, // CTM !!!CZERO added to tell the studiorender that the value in iuser2 is a
                              // lightmultiplier
};

#endif
