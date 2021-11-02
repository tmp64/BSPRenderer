#ifndef SURFACE_ATTRIBS_GLSL
#define SURFACE_ATTRIBS_GLSL

// World-space position
layout (location = 0) in vec3 inPosition;

// World-space normal unit vector
layout (location = 1) in vec3 inNormal;

// Texture coordinates
layout (location = 2) in vec2 inTexCoord;

// Lightmap texture coordinates
layout (location = 3) in vec2 inLMTexCoord;

#endif
