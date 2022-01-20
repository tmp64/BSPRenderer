#include "scene/global_uniform.glsl"

// World-space position
layout (location = 0) in vec3 inPosition;

// World-space normal unit vector
layout (location = 1) in vec3 inNormal;

// Vertex color
layout (location = 2) in vec4 inColor;

// Transformations
layout (location = 3) in mat4 inTransform;

out vec3 gNormal;
out vec4 gColor;

void main(void) {
	gl_Position = u_Global.mMainProj * u_Global.mMainView * inTransform * vec4(inPosition.xyz, 1.0);
	gNormal = inNormal;
	gColor.rgb = pow(inColor.rgb, vec3(u_Global.uflTexGamma));
	gColor.a = inColor.a;
}
