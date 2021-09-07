#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 V;
    mat4 P;
} camera;

layout(set = 2, binding = 0) uniform Model {
    mat4 M;
    mat4 N;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 outTexCoord;

void main() {
    // we still want to apply view matrix, but without movement
    mat4 V = camera.V;
    V[3][0] = 0.0;
    V[3][1] = 0.0;
    V[3][2] = 0.0;

    gl_Position = camera.P * V * model.M * vec4(inPosition, 1.0);
    outTexCoord = inPosition;
}
