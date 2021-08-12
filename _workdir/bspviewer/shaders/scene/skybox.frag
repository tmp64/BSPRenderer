// World-space fragment position
in vec3 gFragPos;

// World-space normal vector
in vec3 gNormal;

// Texture coordinates
in vec3 gTexCoord;

// Output color
out vec4 outColor;

// Uniforms
uniform samplerCube uTexture;
uniform float uTexGamma;

void main(void) {
	// Sample the cubemap
	vec3 skyColor = texture(uTexture, gTexCoord).rgb;

	// Gamma correction
	skyColor.rgb = pow(skyColor.rgb, vec3(uTexGamma));

	// Final color
	outColor = vec4(skyColor, 1.0);
}
