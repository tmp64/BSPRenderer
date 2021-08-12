#ifndef BRUSH_IFACE_H
#define BRUSH_IFACE_H

IFACE_VF ShaderIface {
	vec3 vFragPos;			// World-space fragment position
	vec3 vNormal;			// World-space normal vector
	vec2 vTexCoord;			// Texture coordinates
	vec2 vBSPLMTexCoord;	// BSP lightmap texture coordinates
	vec2 vCustomLMTexCoord;	// Custom lightmap texture coordinates
} vsOut;

#endif
