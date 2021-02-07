in vec2 gTexCoords;

out vec4 outColor;

uniform sampler2D uTexture;

void main() {
	vec3 color = texture(uTexture, gTexCoords).rgb;
	outColor.rgb = color.rgb;
}
