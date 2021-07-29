#include "utils.glsl"

in vec2 gTexCoords;

out vec4 outColor;

uniform sampler2D uHdrBuffer;
uniform int uTonemap;
uniform float uGamma;
uniform float uAvgLum;
uniform float uWhitePoint;

void main() {
	vec3 hdrColor = texture(uHdrBuffer, gTexCoords).rgb;
	vec3 mapped = vec3(1, 0, 1);

	//--------------------------------------------------
	// Tonemapping
	if (uTonemap == 0) {
		// No tonemapping, clamp to 1
		mapped = hdrColor;
	} else if (uTonemap == 1) {
		// yxy.x is Y, the luminance
  		vec3 yxy = convertRGB2Yxy(hdrColor);
		
		// Exposure-adjusted luminance
		float lp = yxy.x / ((9.6 / 1.0) * uAvgLum + 0.0001);

		// Reinhard curve with white point
		yxy.x = reinhardWhitePoint(lp, uWhitePoint);

		// Back to RGB
		mapped = convertYxy2RGB(yxy);
	} else if (uTonemap == 2) {
		// yxy.x is Y, the luminance
  		vec3 yxy = convertRGB2Yxy(hdrColor);
		
		// Exposure-adjusted luminance
		float lp = yxy.x / ((9.6 / 1.0) * uAvgLum + 0.0001);

		// ACES
		yxy.x = aces(lp);

		// Back to RGB
		mapped = convertYxy2RGB(yxy);
	} else {
		// Error
		mapped = hdrColor * vec3(1, 0, 1);
	}
	
	//--------------------------------------------------
	// Apply gamma correction
	outColor.rgb = pow(mapped.rgb, vec3(1.0/uGamma));
}
