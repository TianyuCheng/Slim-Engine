#version 450
#extension GL_ARB_separate_shader_objects : enable

vec2 pos[6] = vec2[6](
    // first triangle
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(+1.0, +1.0),
    // second triangle
    vec2(+1.0, +1.0),
    vec2(-1.0, +1.0),
    vec2(-1.0, -1.0)
);

layout(location = 0) out vec2 outUV;

void main() {
    vec2 P = pos[gl_VertexIndex];

    outUV = P * 0.5 + 0.5;

    gl_Position = vec4(P, 0.0, 1.0);
}
