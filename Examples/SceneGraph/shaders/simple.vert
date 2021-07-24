#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants { mat4 M; } model;

layout(set = 0, binding = 0) uniform Camera { mat4 VP; } camera;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = camera.VP * model.M * vec4(inPosition, 1.0);
}
