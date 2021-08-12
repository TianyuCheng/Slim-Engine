#version 450
#extension GL_ARB_separate_shader_objects : enable

// material set
layout(set = 1, binding = 0) uniform Color {
    vec3 color;
} material;

// output
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(material.color, 1.0);
}
