#version 330 core
in vec2 gTexCoords;
out vec4 outColor;

uniform sampler2D text;
uniform vec4 uTextColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, gTexCoords).r);
    outColor = uTextColor * sampled;
}
