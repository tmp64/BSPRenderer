#include "scene/global_uniform.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoords;

out vec2 gTexCoords;

uniform mat4 u_ModelMatrix;

void main()
{
    gl_Position = u_Global.mMainProj * u_Global.mMainView *
        u_ModelMatrix * vec4(inPosition.xyz, 1.0);

    gTexCoords = inTexCoords;
}
