#version 450
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../common.h"

// samplers
layout(set = 0, binding = GBUFFER_ALBEDO_BINDING) uniform sampler2D albedoImage;
layout(set = 0, binding = GBUFFER_NORMAL_BINDING) uniform sampler2D normalImage;
layout(set = 0, binding = GBUFFER_DEPTH_BINDING)  uniform sampler2D depthImage;
layout(set = 1, binding = SURFEL_DIFFUSE_BINDING) uniform sampler2D diffuseImage;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 albedo  = texture(albedoImage, inUV);
    vec4 diffuse = texture(diffuseImage, inUV);

    // final contribution
    outColor = diffuse * albedo;

    // // gamma correct
    // const float gamma = 2.2;
    // outColor.rgb = pow(outColor.rgb, vec3(1.0/gamma));
}
