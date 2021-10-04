#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "colors.h"

layout(set = 0, binding = 0) uniform usampler2D imageObject;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() {
    uint objectID = texture(imageObject, inUV).x;
    vec3 color = random_colors[objectID % random_color_count];
    outColor = vec4(color, 1.0);
}
