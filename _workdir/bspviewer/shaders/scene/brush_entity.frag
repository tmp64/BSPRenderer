#include "scene/world_iface.glsl"

// Output color
out vec4 outColor;

// Uniforms
uniform vec3 uColor;
uniform sampler2D uTexture;
uniform sampler2D uLMTexture;
uniform int uTextureType;
uniform int uLightingType;
uniform float uTexGamma;
uniform int uRenderMode;
uniform float uFxAmount;
uniform vec3 uFxColor;

#define kRenderNormal		0
#define kRenderTransColor	1
#define kRenderTransTexture	2
#define kRenderGlow			3
#define kRenderTransAlpha	4
#define kRenderTransAdd		5

void main(void) {
	vec4 textureColor = texture(uTexture, vsOut.vTexCoord).rgba;
	vec4 objectColor = vec4(1, 0, 1, 1);
	vec3 ligtmapColor = vec3(1, 1, 1);

	//--------------------------------------------------
	// Surface texture
	if (uTextureType == 0) {
		// White texture
		objectColor = vec4(1, 1, 1, 1);
	} else if (uTextureType == 1) {
		// Random color
		objectColor = vec4(uColor, 1);
	} else if (uTextureType == 2) {
		// Entity texture
		if (uRenderMode == kRenderTransColor) {
			// Entity FX color
			objectColor = vec4(uFxColor, 1);
		} else {
			// Surface texture
			objectColor = textureColor;
		}
	}

	// Texture gamma correction
	objectColor.rgb = pow(objectColor.rgb, vec3(uTexGamma));


	//--------------------------------------------------
	// Surface lighting
	if (uLightingType == 0) {
		// Fullbright
		ligtmapColor = vec3(1, 1, 1);
	} else if (uLightingType == 1) {
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
	} else if (uLightingType == 2) {
		// BSP lightmaps
		ligtmapColor = texture(uLMTexture, vsOut.vBSPLMTexCoord).rgb;
		// Gamma correction
		ligtmapColor.rgb = pow(ligtmapColor.rgb, vec3(1.5));
	} else if (uLightingType == 3) {
		// Custom lightmaps
		ligtmapColor = texture(uLMTexture, vsOut.vCustomLMTexCoord).rgb;
	}

	//--------------------------------------------------
	// Render Mode
	switch (uRenderMode) {
	case kRenderNormal:
		// Normal
		objectColor.a = 1;
		break;
	case kRenderTransColor:
	case kRenderTransTexture:
	case kRenderTransAdd:
		// Color, texture, additive: FX amount controls the alpha
		objectColor.a = uFxAmount;
		
		// Also disable lightmapping
		ligtmapColor = vec3(1, 1, 1);
		break;
	case kRenderGlow:
		// Glow
		// ???
		break;
	case kRenderTransAlpha:
		// Solid
		objectColor.a = objectColor.a * uFxAmount;
		
		// Discard if too transparent
		if (objectColor.a < 0.5) {
			discard;
		}
		
		break;
	}
	
	//objectColor.a = 1;
	
	// Alpha gamma correction
	objectColor.a = pow(objectColor.a, 1.3);

	//--------------------------------------------------
	// Final color
	outColor = objectColor * vec4(ligtmapColor, 1.0);
}
