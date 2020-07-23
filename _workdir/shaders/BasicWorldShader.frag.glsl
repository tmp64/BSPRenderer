#version 130

//in vec2 texCoords;

out vec4 outColor;

//uniform sampler2D textureSampler;
uniform vec3 color;

void main(void)
{
	outColor = vec4(color, 1.0);
	//outColor = texture(textureSampler, texCoords);
}
