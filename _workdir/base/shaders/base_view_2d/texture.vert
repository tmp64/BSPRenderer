layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoords;

uniform mat4 uProjMatrix;
uniform mat4 uViewMatrix;

out vec2 gTexCoords;

void main()
{
	gl_Position = uProjMatrix * uViewMatrix * vec4(inPosition.xyz, 1.0);
    gTexCoords = inTexCoords;
}
