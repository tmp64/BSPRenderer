#include "scene/global_uniform.glsl"

//#define RGB_BRIGHTNESS vec3(0.2126, 0.7152, 0.0722)
#define RGB_BRIGHTNESS vec3(0.25, 0.50, 0.25)

// For when I fix texlight brightness
//const float DESAT_START_VALUE = 0.86;
//const float DESAT_START = 4;
//const float DESAT_END = 5;

const float DESAT_START_VALUE = 0.86;
const float DESAT_START = 16;
const float DESAT_END = 44;

const float DESAT_K = log(1 - DESAT_START_VALUE) / (DESAT_START - DESAT_END);
const float DESAT_B = DESAT_K * DESAT_END;

in vec2 gTexCoords;

out vec4 outColor;

uniform sampler2D u_HdrBuffer;

//! @returns saturation of color of brightness x.
float desaturate(float x) {
	return clamp(1 - exp(DESAT_K * x - DESAT_B), 0, 1);
}

void main() {
	// Read HDR color
	vec3 hdrColor = texture(u_HdrBuffer, gTexCoords).rgb;
	float brightness = dot(hdrColor, RGB_BRIGHTNESS);
	
	// Desaturate bright colors
	float saturation = desaturate(brightness);
	hdrColor = mix(vec3(brightness), hdrColor, saturation);

	// Clamp it to [0; 1]
	hdrColor = clamp(hdrColor, vec3(0), vec3(1));

	// Apply gamma correction
	outColor.rgb = pow(hdrColor.rgb, vec3(1.0 / u_Global.uflGamma));
}
