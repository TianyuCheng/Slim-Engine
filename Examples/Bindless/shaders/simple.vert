#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable

layout (push_constant) uniform Object {
    int objectId;
} object;

layout (set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
} camera;

// descriptor indexing
layout (set = 0, binding = 1) uniform Model {
    mat4 model;
} models[];

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 outColor;

void main() {
    mat4 mvp = camera.proj * camera.view * models[nonuniformEXT(object.objectId)].model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    outColor = inColor;
}
