in vec2 gTexCoords;

uniform sampler2D uHdrBuffer;
out vec4 outColor;

void main() {
    vec3 hdrColor = texture(uHdrBuffer, gTexCoords).rgb;
    float l = dot(hdrColor, vec3(0.2125, 0.7154, 0.0721));
    outColor.r = log(0.05 + l);
}
