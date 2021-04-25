// World-space position
layout (location = 0) in vec3 inPosition;

// World-space normal unit vector
layout (location = 1) in vec3 inNormal;

// Texture coordinates
layout (location = 2) in vec2 inTexCoord;

// BSP lightmap texture coordinates
layout (location = 3) in vec2 inBSPLMTexCoord;

// Custom lightmap texture coordinates
layout (location = 4) in vec2 inCustomLMTexCoord;

out vec3 gFragPos;
out vec3 gNormal;
out vec2 gTexCoord;
out vec2 gBSPLMTexCoord;
out vec2 gCustomLMTexCoord;

uniform mat4 uProjMatrix;
uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;

void main(void) {
	gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * vec4(inPosition.xyz, 1.0);
	gFragPos = inPosition;
	gNormal = inNormal;
	gTexCoord = inTexCoord;
	gBSPLMTexCoord = inBSPLMTexCoord;
	gCustomLMTexCoord = inCustomLMTexCoord;
}
