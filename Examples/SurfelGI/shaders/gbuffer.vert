#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Object {
    uint instanceID;
    uint baseColorTextureID;
    uint baseColorSamplerID;
} object;

layout(set = 0, binding = 0) uniform Camera {
    mat4 V;
    mat4 P;
} camera;

layout(set = 0, binding = 1) buffer Transforms {
    mat4 M[];
} transforms;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec2 outUV;

void main() {
    mat4 M = transforms.M[object.instanceID];
    gl_Position = camera.P * camera.V * M * vec4(inPosition, 1.0);

    outWorldPos     = vec3(M * vec4(inPosition, 1.0));
    outWorldNormal  = vec3(M * vec4(inNormal,   0.0));
    outUV           = inUV;
}
