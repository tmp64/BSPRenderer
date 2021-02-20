// World-space position
layout (location = 0) in vec3 inPosition;

// World-space normal unit vector
layout (location = 1) in vec3 inNormal;

// Texture coordinates
layout (location = 2) in vec2 inTexCoord;

out vec3 gFragPos;
out vec3 gNormal;
out vec3 gTexCoord;

uniform mat4 uProjMatrix;
uniform mat4 uViewMatrix;
uniform vec3 uViewOrigin;

void main(void) {
	vec4 position = uProjMatrix * uViewMatrix * vec4(inPosition.xyz, 1.0);
	gl_Position = position.xyww;	// Make depth = 1 so it doesn't draw over real geometry
	gFragPos = inPosition;
	gNormal = inNormal;
	
	// Coords are swapped to orient cubemap properly
	gTexCoord = inPosition.xzy - uViewOrigin.xzy;
}
