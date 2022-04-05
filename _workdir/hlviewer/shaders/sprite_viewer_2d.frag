in vec2 gTexCoords;
out vec4 outColor;

uniform sampler2DArray u_Sprite;
uniform float u_Frame;
uniform vec3 u_Color;

void main() {
	vec4 sprite = texture(u_Sprite, vec3(gTexCoords, u_Frame)).rgba;
	outColor = sprite * vec4(u_Color, 1.0);
}
