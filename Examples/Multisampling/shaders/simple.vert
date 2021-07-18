#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 mvp;
} camera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;

void main() {
    gl_Position = camera.mvp * vec4(inPosition, 1.0);
    outColor = inColor;
}
