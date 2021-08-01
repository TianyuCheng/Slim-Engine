#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UBOView {
    mat4 proj;
    mat4 view;
} uboView;

layout(set = 1, binding = 0) uniform UboInstance {
    mat4 model;
} uboInstance;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;

void main() {
    mat4 mvp = uboView.proj * uboView.view * uboInstance.model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    outColor = inColor;
}
