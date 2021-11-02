#include "scene/brush_iface.glsl"
#include "scene/global_uniform.glsl"

// Output color
out vec4 outColor;

// Uniforms
uniform vec3 u_Color;
uniform sampler2D u_Texture;
uniform sampler2D u_LMTexture;
uniform vec4 u_TintColor;

#ifdef ENTITY_SHADER
uniform int u_iRenderMode;
uniform float u_flFxAmount;
uniform vec3 u_vFxColor;

#define kRenderNormal		0
#define kRenderTransColor	1
#define kRenderTransTexture	2
#define kRenderGlow			3
#define kRenderTransAlpha	4
#define kRenderTransAdd		5
#endif

void main(void) {
	vec4 textureColor = texture(u_Texture, vsOut.vTexCoord).rgba;
	vec4 objectColor = vec4(1, 0, 1, 1);
	vec3 ligtmapColor = vec3(1, 1, 1);

	//--------------------------------------------------
	// Surface texture
	if (u_Global.uiTextureType == 0) {
		// White texture
		objectColor = vec4(1, 1, 1, 1);
	} else if (u_Global.uiTextureType == 1) {
		// Random color
		objectColor = vec4(u_Color, 1);
	} else if (u_Global.uiTextureType == 2) {
		// Entity texture
#ifdef ENTITY_SHADER
		if (u_iRenderMode == kRenderTransColor) {
			// Entity FX color
			objectColor = vec4(u_vFxColor, 1);
		} else
#endif
        {
			// Surface texture
			objectColor = textureColor;
		}
	}

	// Texture gamma correction
	objectColor.rgb = pow(objectColor.rgb, vec3(u_Global.uflTexGamma));


	//--------------------------------------------------
	// Surface lighting
	if (u_Global.uiLightingType == 0) {
		// Fullbright
		ligtmapColor = vec3(1, 1, 1);
	} else if (u_Global.uiLightingType == 1) {
		// Shaded lighting
		// Ambient lighting
		vec3 ambient = vec3(1, 1, 1) * 0.75;
		
		// Direct diffuse ligthing
		vec3 normal = normalize(vsOut.vNormal);
		vec3 lightDir = normalize(vec3(1.0, 1.5, 1.5));
		
		float diff = max(dot(normal, lightDir), 0.0);
		vec3 diffuseLightColor = vec3(1, 1, 1) * 0.3;
		vec3 diffuse = diff * diffuseLightColor;
		
		// Result
		ligtmapColor = ambient + diffuse;
	} else if (u_Global.uiLightingType == 2) {
		// BSP lightmaps
		ligtmapColor = texture(u_LMTexture, vsOut.vBSPLMTexCoord).rgb;
		// Gamma correction
		ligtmapColor.rgb = pow(ligtmapColor.rgb, vec3(1.5));
	} else if (u_Global.uiLightingType == 3) {
		// Custom lightmaps
		ligtmapColor = texture(u_LMTexture, vsOut.vCustomLMTexCoord).rgb;
	}

#ifdef ENTITY_SHADER
	//--------------------------------------------------
	// Render Mode
	switch (u_iRenderMode) {
	case kRenderNormal:
		// Normal
		objectColor.a = 1;
		break;
	case kRenderTransColor:
	case kRenderTransTexture:
	case kRenderTransAdd:
		// Color, texture, additive: FX amount controls the alpha
		objectColor.a = u_flFxAmount;
		
		// Also disable lightmapping
		ligtmapColor = vec3(1, 1, 1);
		break;
	case kRenderGlow:
		// Glow
		// ???
		break;
	case kRenderTransAlpha:
		// Solid
		objectColor.a = objectColor.a * u_flFxAmount;
		
		// Discard if too transparent
		if (objectColor.a < 0.5) {
			discard;
		}
		
		break;
	}
	
	// Alpha gamma correction
	objectColor.a = pow(objectColor.a, 1.3);
#else
    objectColor.a = 1;
#endif

	//--------------------------------------------------
	// Final color
	outColor = objectColor * vec4(ligtmapColor, 1.0);
	outColor = vec4(mix(outColor.rgb, u_TintColor.rgb, u_TintColor.a), outColor.a);
}
