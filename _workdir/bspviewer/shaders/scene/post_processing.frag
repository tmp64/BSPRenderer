in vec2 gTexCoords;

out vec4 outColor;

uniform sampler2D uHdrBuffer;
uniform sampler2D uLumBuffer;
uniform float uGamma;

void main() {
	// Read HDR color
	vec3 hdrColor = texture(uHdrBuffer, gTexCoords).rgb;
	
	// Clamp it to [0; 1]
	hdrColor = clamp(hdrColor, vec3(0), vec3(1));

	// Apply gamma correction
	outColor.rgb = pow(hdrColor.rgb, vec3(1.0/uGamma));
}
