#version 330 core
layout (location = 0) in vec4 inVertex; // <vec2 pos, vec2 tex>
out vec2 gTexCoords;

uniform mat4 uProjection;
uniform vec4 uPosition;

void main()
{
	vec4 vertex = uPosition + inVertex;
    gl_Position = uProjection * vec4(vertex.xy, 0.0, 1.0);
    gTexCoords = inVertex.zw;
} 
