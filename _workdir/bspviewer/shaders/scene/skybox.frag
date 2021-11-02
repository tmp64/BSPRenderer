#include "scene/global_uniform.glsl"

// Texture coordinates
in vec3 gTexCoord;

#ifdef SUPPORT_TINTING
in vec4 gTintColor;
#endif

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
#ifdef SUPPORT_TINTING
	outColor = vec4(mix(skyColor, gTintColor.rgb, gTintColor.a), 1.0);
#else
	outColor = vec4(skyColor, 1.0);
#endif
}
