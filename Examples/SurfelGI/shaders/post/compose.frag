#version 450
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../common.h"

layout(push_constant) uniform Control { SurfelDebugControl data; } control;

// samplers
layout(set = 0, binding = GBUFFER_ALBEDO_BINDING)   uniform sampler2D albedoImage;
layout(set = 0, binding = GBUFFER_NORMAL_BINDING)   uniform sampler2D normalImage;
layout(set = 0, binding = GBUFFER_DEPTH_BINDING)    uniform sampler2D depthImage;
layout(set = 0, binding = GBUFFER_DIFFUSE_BINDING)  uniform sampler2D diffuseImage;
/* layout(set = 0, binding = GBUFFER_SPECULAR_BINDING) uniform sampler2D specularImage; */
layout(set = 1, binding = DEBUG_SURFEL_BINDING)     uniform sampler2D debugImage;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 albedo  = texture(albedoImage, inUV);
    vec4 diffuse = texture(diffuseImage, inUV);

    // final contribution
    outColor = diffuse * albedo;
    // outColor = diffuse;

    // show debug points
    if (control.data.debugPoint != 0) {
        vec4 debug = texture(debugImage, inUV);
        outColor += debug;
    }

    // // gamma correct
    // const float gamma = 2.2;
    // outColor.rgb = pow(outColor.rgb, vec3(1.0/gamma));
}
