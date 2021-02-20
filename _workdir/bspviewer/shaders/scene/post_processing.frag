in vec2 gTexCoords;

out vec4 outColor;

uniform sampler2D uHdrBuffer;
uniform int uTonemap;
uniform float uExposure;
uniform float uGamma;

void main() {
	vec3 hdrColor = texture(uHdrBuffer, gTexCoords).rgb;
	vec3 mapped = vec3(1, 0, 1);

	//--------------------------------------------------
	// Tonemapping
	if (uTonemap == 0) {
		// No tonemapping, clamp to 1
		mapped = hdrColor;
	} else if (uTonemap == 1) {
		// Reinhard tone mapping
		mapped = hdrColor / (hdrColor + vec3(1.0));
	} else if (uTonemap == 2) {
		// Exposure
		mapped = vec3(1.0) - exp(-hdrColor * uExposure);
	}
	
	//--------------------------------------------------
	// Apply gamma correction
	outColor.rgb = pow(mapped.rgb, vec3(1.0/uGamma));
}
