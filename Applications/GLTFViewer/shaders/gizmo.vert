#version 450
#extension GL_ARB_separate_shader_objects : enable

// camera set
layout(set = 0, binding = 0) uniform Camera {
    mat4 V;
    mat4 P;
} camera;

// model set
layout(set = 2, binding = 0) uniform Model {
    mat4 M;
    mat3 N;
} model;

// attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;


void main() {
    gl_Position = camera.P * camera.V * model.M * vec4(inPosition, 1.0);
}
