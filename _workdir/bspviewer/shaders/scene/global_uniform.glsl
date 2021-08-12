#ifndef SCENE_GLOBAL_UNIFORM_GLSL
#define SCENE_GLOBAL_UNIFORM_GLSL

//! Global uniform data
//! Must match SceneRenderer::GlobalUniform
layout (std140) uniform GlobalUniform {
    mat4 mMainProj;
    mat4 mMainView;
    vec4 vMainViewOrigin; // xyz
    vec4 vflParams1;      // x tex gamma, y screen gamma, z sim time, w sim time delta
    ivec4 viParams1;      // x is texture type, y is lighting type
} u_Global;

#define uflTexGamma     vflParams1.x
#define uflGamma        vflParams1.y
#define uflSimTime      vflParams1.z
#define uflDeltaTime    vflParams1.w
#define uiTextureType   viParams1.x
#define uiLightingType  viParams1.y

#endif
