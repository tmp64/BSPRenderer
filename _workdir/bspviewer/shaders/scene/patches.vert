#include "scene/global_uniform.glsl"

// World-space position
layout (location = 0) in vec3 inPosition;

void main(void) {
	gl_Position = u_Global.mMainProj * u_Global.mMainView * vec4(inPosition.xyz, 1.0);
}
