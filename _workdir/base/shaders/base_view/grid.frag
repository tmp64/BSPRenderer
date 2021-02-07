uniform vec3 uColor;

// Output color
out vec4 outColor;

void main(void) {
	outColor = vec4(uColor.rgb, 1.0);
}
