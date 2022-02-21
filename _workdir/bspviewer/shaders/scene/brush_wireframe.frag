#include "scene/global_uniform.glsl"

// Output color
out vec4 outColor;

uniform vec3 u_vColor;

void main(void) {
	gl_FragDepth = gl_FragCoord.z - 0.00005;
	outColor = vec4(pow(u_vColor, vec3(u_Global.uflTexGamma)), 1.0);
}
