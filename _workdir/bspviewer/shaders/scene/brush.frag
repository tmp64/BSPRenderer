#include "scene/brush_iface.glsl"
#include "scene/global_uniform.glsl"
#include "scene/rendermode.glsl"

// Output color
out vec4 outColor;

// Uniforms
uniform vec3 u_Color;
uniform sampler2D u_Texture;
uniform sampler2DArray u_LMTexture;

#ifdef ENTITY_SHADER
uniform int u_iRenderMode;
uniform float u_flFxAmount;
uniform vec3 u_vFxColor;
#endif

vec3 sampleLightmap(int i, float scale) {
	vec3 lightmapColor = texture(u_LMTexture, vec3(vsOut.vLMTexCoord, i)).rgb;
	return pow(lightmapColor.rgb, vec3(u_Global.uflLMGamma)) * scale;
}

void main(void) {
	vec4 textureColor = texture(u_Texture, vsOut.vTexCoord).rgba;
	vec4 objectColor = vec4(1, 0, 1, 1);
	vec3 lightmapColor = vec3(1, 1, 1);

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
		lightmapColor = vec3(1, 1, 1);
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
		lightmapColor = ambient + diffuse;
	} else if (u_Global.uiLightingType == 2) {
		// Lightmaps
		lightmapColor = vec3(0, 0, 0);
		lightmapColor += sampleLightmap(0, vsOut.vLightstyleScale.x);
		lightmapColor += sampleLightmap(1, vsOut.vLightstyleScale.y);
		lightmapColor += sampleLightmap(2, vsOut.vLightstyleScale.z);
		lightmapColor += sampleLightmap(3, vsOut.vLightstyleScale.w);
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
		lightmapColor = vec3(1, 1, 1);
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
	outColor = objectColor * vec4(lightmapColor, 1.0);
	
#ifdef SUPPORT_TINTING
	outColor = vec4(mix(outColor.rgb, vsOut.vTintColor.rgb, vsOut.vTintColor.a), outColor.a);
#endif
}
