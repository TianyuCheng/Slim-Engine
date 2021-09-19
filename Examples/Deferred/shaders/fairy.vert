#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 V;
    mat4 P;
} camera;

layout(location = 0) in vec3 inVertexPosition;
layout(location = 1) in vec3 inFairyPosition;
layout(location = 2) in vec3 inFairyColor;

layout(location = 0) out vec3 outFairyColor;

void main() {
    float inFairyRadius = 3.0;

    // instanced transform
    mat4 M = mat4(1.0);
    M[0][0] = inFairyRadius;
    M[1][1] = inFairyRadius;
    M[2][2] = inFairyRadius;
    M[3][0] = inFairyPosition.x;
    M[3][1] = inFairyPosition.y;
    M[3][2] = inFairyPosition.z;

    mat4 mvp = camera.P * camera.V * M;
    gl_Position = mvp * vec4(inVertexPosition, 1.0);
    outFairyColor = inFairyColor;
}
