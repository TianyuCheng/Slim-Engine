#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable

layout (push_constant) uniform Material {
    int materialId;
} constants;

// least frequently updated
layout (set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
} camera;

// most frequently updated
layout (set = 2, binding = 0) uniform Model {
    mat4 model;
} models;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) flat out int outMaterialId;
layout (location = 1) out vec2 outTexCoord;

void main() {
    mat4 mvp = camera.proj * camera.view * models.model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    outMaterialId = constants.materialId;
    outTexCoord = inTexCoord;
}
