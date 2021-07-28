#include "scene/surface_attribs.glsl"
#include "scene/world_iface.glsl"

uniform mat4 uProjMatrix;
uniform mat4 uViewMatrix;

void main(void) {
	gl_Position = uProjMatrix * uViewMatrix * vec4(inPosition.xyz, 1.0);
	vsOut.vFragPos = inPosition;
	vsOut.vNormal = inNormal;
	vsOut.vTexCoord = inTexCoord;
	vsOut.vBSPLMTexCoord = inBSPLMTexCoord;
	vsOut.vCustomLMTexCoord = inCustomLMTexCoord;
}
