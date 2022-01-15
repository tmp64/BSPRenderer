#include "scene/global_uniform.glsl"

// World-space position
layout (location = 0) in vec3 inPosition;

void main(void) {
	// Shift the position a bit towards the camera
	vec3 pos = inPosition.xyz;
	pos = pos + normalize(u_Global.vMainViewOrigin.xyz - pos) * 0.3;
	
	gl_Position = u_Global.mMainProj * u_Global.mMainView * vec4(pos.xyz, 1.0);
}
