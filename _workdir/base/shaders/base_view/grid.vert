// World-space position
layout (location = 0) in vec3 inPosition;

uniform mat4 uProjMatrix;
uniform mat4 uViewMatrix;

void main(void) {
	gl_Position = uProjMatrix * uViewMatrix * vec4(inPosition.xyz, 1.0);
}
