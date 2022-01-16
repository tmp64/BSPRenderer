uniform sampler2D Texture;

in vec2 Frag_UV;
in vec4 Frag_Color;

out vec4 Out_Color;

void main() {
    vec4 texColor = texture(Texture, Frag_UV.st);
    texColor.rgb = pow(texColor.rgb, vec3(GAMMA));
    Out_Color = Frag_Color * texColor;
};
