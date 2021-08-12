#include "scene/global_uniform.glsl"

// Texture coordinates
in vec3 gTexCoord;

// Output color
out vec4 outColor;

// Uniforms
uniform samplerCube u_Texture;

void main(void) {
	// Sample the cubemap
	vec3 skyColor = texture(u_Texture, gTexCoord).rgb;

	// Gamma correction
	skyColor.rgb = pow(skyColor.rgb, vec3(u_Global.uflTexGamma));

	// Final color
	outColor = vec4(skyColor, 1.0);
}
