#include "scene/surface_attribs.glsl"
#include "scene/global_uniform.glsl"

out vec3 gTexCoord;

#ifdef SUPPORT_TINTING
out vec4 gTintColor;
#endif

void main(void) {
	vec4 position = u_Global.mMainProj * u_Global.mMainView * vec4(inPosition.xyz, 1.0);
	gl_Position = position.xyww;	// Make depth = 1 so it doesn't draw over real geometry
	
	// Coords are swapped to orient cubemap properly
	gTexCoord = inPosition.xzy - u_Global.vMainViewOrigin.xzy;

#ifdef SUPPORT_TINTING
	gTintColor = inTintColor;
#endif
}
