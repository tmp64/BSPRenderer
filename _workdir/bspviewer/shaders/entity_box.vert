#include "scene/global_uniform.glsl"

// World-space position
layout (location = 0) in vec3 inPosition;

// World-space normal unit vector
layout (location = 1) in vec3 inNormal;

// Transformations
layout (location = 2) in mat4 inTransform;

out vec3 gNormal;

void main(void) {
	gl_Position = u_Global.mMainProj * u_Global.mMainView * inTransform * vec4(inPosition.xyz, 1.0);
	gNormal = inNormal;
}
