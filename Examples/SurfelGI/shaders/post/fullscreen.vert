#version 450
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_separate_shader_objects : enable

// for drawing a quad
const vec2 vertices[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(+1.0, +1.0),
    vec2(+1.0, +1.0),
    vec2(-1.0, +1.0),
    vec2(-1.0, -1.0)
);

layout(location = 0) out vec2 outUV;

void main() {
    vec2 pos = vertices[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);

    outUV = pos * 0.5 + 0.5;
    outUV.y = -outUV.y;
}
