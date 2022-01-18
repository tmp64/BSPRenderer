uniform mat4 ProjMtx;

layout (location = 0) in vec2 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec4 Color;

out vec2 Frag_UV;
out vec4 Frag_Color;

void main()
{
    Frag_UV = UV;
    Frag_Color = vec4(pow(Color.rgb, vec3(GAMMA)), Color.a);
    gl_Position = ProjMtx * vec4(Position.xy,0,1);
};