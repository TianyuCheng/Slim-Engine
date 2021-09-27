#version 450
#extension GL_ARB_separate_shader_objects : enable

// for drawing a quad
const vec3 colors[12] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 0.0, 1.0),
    vec3(0.0, 1.0, 1.0),
    vec3(0.2, 0.5, 0.7),
    vec3(0.5, 0.3, 0.1),
    vec3(0.6, 0.4, 0.2),
    vec3(0.4, 0.1, 0.9),
    vec3(0.5, 0.3, 0.7),
    vec3(0.1, 0.7, 0.2)
);

layout(set = 0, binding = 0) uniform usampler2D imageObject;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() {
    uint objectID = texture(imageObject, inUV).x;
    vec3 color = colors[objectID % 12];
    outColor = vec4(color, 1.0);
}
