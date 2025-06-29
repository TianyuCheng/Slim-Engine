#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in  vec3 inNormal;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(normalize(inNormal) * 0.5 + 0.5, 1.0);
}
