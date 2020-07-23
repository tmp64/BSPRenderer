#version 130

in vec3 position;
//in vec2 textureCoords;

//out vec2 texCoords;

uniform mat4 transMatrix;
uniform mat4 projMatrix;
uniform mat4 viewMatrix;

void main(void)
{
	vec4 a;
	a = projMatrix * viewMatrix * transMatrix * vec4(position.xyz, 1.0);
	
	/*if (a.x < -1)
		a.x = -1;
	if (a.x > 1)
		a.x = 1;
		
	if (a.y < -1)
		a.y = -1;
	if (a.y > 1)
		a.y = 1;
		
	if (a.z < -1)
		a.z = -1;
	if (a.z > 1)
		a.z = 1;*/
		
	//a = vec4(position.xy, 0.0, 1.0);
	gl_Position = a;
	//color = vec3(position.x + 0.5, position.x+0.5, (position.y + 0.5) / 2.0);
	//texCoords = textureCoords;
	//gl_Position = vec4(position.xyz, 1.0);
}
