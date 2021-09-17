#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 2, binding = 0) uniform sampler2D matcap;

layout(location = 0) in  vec3 inNormal;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 matcapUV = (inNormal * 0.5 + 0.5).xy;
    vec4 color = texture(matcap, matcapUV);
    outColor = color;
}
