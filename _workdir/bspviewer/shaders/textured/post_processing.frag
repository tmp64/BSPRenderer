in vec2 gTexCoords;

out vec4 outColor;

uniform sampler2D uHdrBuffer;
uniform float uExposure;
uniform float uGamma;

void main() {
	vec3 hdrColor = texture(uHdrBuffer, gTexCoords).rgb;

	// Reinhard tone mapping
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
	
	// Exposure
	//const float exposure = 0.5;
	//vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

	// Apply gamma correction
	outColor.rgb = pow(mapped.rgb, vec3(1.0/uGamma));
}
