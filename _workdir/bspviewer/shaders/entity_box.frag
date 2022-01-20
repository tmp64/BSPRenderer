#include "scene/global_uniform.glsl"

// Normal vector
in vec3 gNormal;

// color
in vec4 gColor;

// Output color
out vec4 outColor;


void main(void) {
    // Shaded lighting
    // Ambient lighting
    vec3 ambient = vec3(1, 1, 1) * 0.75;
    
    // Direct diffuse ligthing
    vec3 normal = normalize(gNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.5, 1.5));
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuseLightColor = vec3(1, 1, 1) * 0.3;
    vec3 diffuse = diff * diffuseLightColor;
    
    // Result lighting
    vec3 ligtmapColor = ambient + diffuse;

	// Final color
	outColor = vec4(gColor.rgb * ligtmapColor, 1.0);
}
