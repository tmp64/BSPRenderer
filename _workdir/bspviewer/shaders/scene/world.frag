// World-space fragment position
in vec3 gFragPos;

// World-space normal vector
in vec3 gNormal;

// Texture coordinates
in vec2 gTexCoord;

// BSP lightmap texture coordinates
in vec2 gBSPLMTexCoord;

// Custom lightmap texture coordinates
in vec2 gCustomLMTexCoord;

// Output color
out vec4 outColor;

// Uniforms
uniform vec3 uColor;
uniform sampler2D uTexture;
uniform sampler2D uLMTexture;
uniform int uTextureType;
uniform int uLightingType;
uniform float uTexGamma;

void main(void) {
	vec3 objectColor = vec3(1, 0, 1);
	vec3 ligtmapColor = vec3(1, 1, 1);

	//--------------------------------------------------
	// Surface texture
	if (uTextureType == 0) {
		// White texture
		objectColor = vec3(1, 1, 1);
	} else if (uTextureType == 1) {
		// Random color
		objectColor = uColor;
	} else if (uTextureType == 2) {
		// Texture
		objectColor = texture(uTexture, gTexCoord).rgb;
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
		vec3 normal = normalize(gNormal);
		vec3 lightDir = normalize(vec3(1.0, 1.5, 1.5));
		
		float diff = max(dot(normal, lightDir), 0.0);
		vec3 diffuseLightColor = vec3(1, 1, 1) * 0.3;
		vec3 diffuse = diff * diffuseLightColor;
		
		// Result
		ligtmapColor = ambient + diffuse;
	} else if (uLightingType == 2) {
		// BSP lightmaps
		ligtmapColor = texture(uLMTexture, gBSPLMTexCoord).rgb;
	} else if (uLightingType == 3) {
		// Custom lightmaps
		ligtmapColor = texture(uLMTexture, gCustomLMTexCoord).rgb;
	}

	//--------------------------------------------------
	// Final color
	outColor = vec4(objectColor * ligtmapColor, 1.0);
}
