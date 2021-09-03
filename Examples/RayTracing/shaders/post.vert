#version 450

vec2 positions[6] = vec2[](
    // tri 1
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    // tri 2
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0),
    vec2(-1.0, -1.0)
);

layout(location = 0) out vec2 outUV;

void main()
{
    vec2 position = positions[gl_VertexIndex];
    outUV         = position * 0.5 + 0.5;
    gl_Position   = vec4(position, 0.0, 1.0);
}
