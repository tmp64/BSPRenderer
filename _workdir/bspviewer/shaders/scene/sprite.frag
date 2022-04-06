#include "scene/global_uniform.glsl"
#include "scene/rendermode.glsl"

// Texture coords
in vec2 gTexCoords;

// Output color
out vec4 outColor;

uniform sampler2DArray u_Texture;
uniform float u_flSpriteFrame;
uniform int u_iRenderMode;
uniform float u_flFxAmount;
uniform vec3 u_vFxColor;

void main(void) {
    vec4 textureColorGamma = texture(u_Texture, vec3(gTexCoords, u_flSpriteFrame)).rgba; //!< Sampled texture in gamma space
	vec4 objectColor = vec4(1, 0, 1, 1); //!< Color in linear space

    //--------------------------------------------------
    // Gamma correction
    objectColor.rgb = pow(textureColorGamma.rgb, vec3(u_Global.uflTexGamma));
    objectColor.a = textureColorGamma.a;
    vec3 fxColor = pow(u_vFxColor, vec3(u_Global.uflTexGamma));

    //--------------------------------------------------
	// Render Mode
	switch (u_iRenderMode) {
	case kRenderNormal:
		// Normal
        // FX Color affects the overall color of the sprite. FX Amount is ignored.
		objectColor.rgb *= fxColor;
		break;
	case kRenderTransColor:
        // Color
        // FX Color is ignored. FX Amount is used as a visibility flag (0 is invisible, any other value is visible)
        objectColor.a *= float(u_flFxAmount > 0.0);
        break;
	case kRenderTransTexture:
	case kRenderGlow:
	case kRenderTransAlpha:
	case kRenderTransAdd:
        // Texture, Glow, Solid, Additive
        // FX Color affects the overall color, and FX Amount the overall transparency of the sprite
        objectColor *= vec4(fxColor, u_flFxAmount);
        break;
	}

    //--------------------------------------------------
    // Output color
    outColor = objectColor;
}