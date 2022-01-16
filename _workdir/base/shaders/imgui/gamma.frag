uniform sampler2D Texture;

in vec2 Frag_UV;
in vec4 Frag_Color;

out vec4 Out_Color;

void main() {
    vec4 texColor = texture(Texture, Frag_UV.st);
    Out_Color = Frag_Color * texColor;
};
