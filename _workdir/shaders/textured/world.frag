#version 330 core

// World-space fragment position
in vec3 gFragPos;

// World-space normal vector
in vec3 gNormal;

// Texture coordinates
in vec2 gTexCoord;

// Lightmap texture coordinates
in vec2 gLMTexCoord;

// Output color
out vec4 outColor;

// Uniforms
uniform vec3 uColor;
uniform sampler2D uTexture;
uniform sampler2D uLightmap;
uniform int uFullBright;
uniform float uGamma;
uniform float uTexGamma;

void main(void) {
	vec3 objectColor = texture(uTexture, gTexCoord).xyz;
	vec3 ligtmapColor = texture(uLightmap, gLMTexCoord).xyz;
	
	objectColor.rgb = pow(objectColor.rgb, vec3(uTexGamma));
	
	if (uFullBright == 1) {
		// Disable lighting
		outColor = vec4(objectColor, 1.0);
		return;
	}
	
	if (uFullBright == 2) {
		// Disable textures
		objectColor = vec3(0.8, 0.8, 0.8);
	}
	
	if (uFullBright == 3) {
		// Only use polygon color
		outColor = vec4(uColor.rgb, 1.0);
		return;
	}
	
	if (uFullBright == 4) {
		// Ambient lighting
		vec3 ambient = vec3(1, 1, 1) * 0.75;
		
		// Diffuse ligthing
		vec3 normal = normalize(gNormal);
		vec3 lightDir = normalize(vec3(1.0, 1.5, 1.5));
		
		float diff = max(dot(normal, lightDir), 0.0);
		vec3 diffuseLightColor = vec3(1, 1, 1) * 0.3;
		vec3 diffuse = diff * diffuseLightColor;
		
		// Result
		vec3 result = (ambient + diffuse) * objectColor;
		outColor = vec4(result, 1.0);
		return;
	}
	
	outColor = vec4(objectColor.r * ligtmapColor.r, objectColor.g * ligtmapColor.g, objectColor.b * ligtmapColor.b, 1.0);
	
	// Apply gamma correction
	outColor.rgb = pow(outColor.rgb, vec3(1.0/uGamma));
}
