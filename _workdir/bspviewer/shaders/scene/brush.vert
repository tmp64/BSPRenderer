#include "scene/surface_attribs.glsl"
#include "scene/brush_iface.glsl"
#include "scene/global_uniform.glsl"

#ifdef ENTITY_SHADER
uniform mat4 u_ModelMatrix;
#endif

void main(void) {
	gl_Position =
        u_Global.mMainProj *
        u_Global.mMainView *
#ifdef ENTITY_SHADER
        u_ModelMatrix *
#endif
        vec4(inPosition.xyz, 1.0);

	vsOut.vFragPos = inPosition;
	vsOut.vNormal = inNormal;
	vsOut.vTexCoord = inTexCoord;
	vsOut.vLMTexCoord = inLMTexCoord;
}
