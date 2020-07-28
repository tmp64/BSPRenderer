#version 330 core

// World-space position
layout (location = 0) in vec3 inPosition;

// World-space normal unit vector
layout (location = 1) in vec3 inNormal;

// Texture coordinates
layout (location = 2) in vec2 inTexCoord;

out vec3 gFragPos;
out vec3 gNormal;
out vec2 gTexCoord;

uniform mat4 uProjMatrix;
uniform mat4 uViewMatrix;

void main(void) {
	gl_Position = uProjMatrix * uViewMatrix * vec4(inPosition.xyz, 1.0);
	gFragPos = inPosition;
	gTexCoord = inTexCoord;
	gNormal = inNormal;
}
