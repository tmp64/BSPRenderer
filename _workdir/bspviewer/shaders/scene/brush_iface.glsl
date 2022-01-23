#ifndef BRUSH_IFACE_H
#define BRUSH_IFACE_H

IFACE_VF ShaderIface {
	vec3 vFragPos;			// World-space fragment position
	vec3 vNormal;			// World-space normal vector
	vec2 vTexCoord;			// Texture coordinates
	vec4 vLightstyleScale;	// Lightstyle intensity
	vec2 vLMTexCoord;		// Lightmap texture coordinates

#ifdef SUPPORT_TINTING
	vec4 vTintColor;		// Surface tint color
#endif
} vsOut;

#endif
