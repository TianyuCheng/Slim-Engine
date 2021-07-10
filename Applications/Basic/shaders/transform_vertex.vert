#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform Camera {
    mat4 mvp;
} camera;

void main() {
    gl_Position = camera.mvp * vec4(inPosition, 1.0);
}
