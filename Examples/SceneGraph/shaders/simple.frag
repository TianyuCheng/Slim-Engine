#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform Color { vec3 albedo; } color;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(color.albedo, 1.0);
}
