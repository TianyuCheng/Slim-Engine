#version 450
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../common.h"

layout(push_constant) uniform Control { DebugControl data; } control;

// samplers
layout(set = 0, binding = GBUFFER_ALBEDO_BINDING)   uniform sampler2D albedoImage;
layout(set = 0, binding = GBUFFER_NORMAL_BINDING)   uniform sampler2D normalImage;
layout(set = 0, binding = GBUFFER_DEPTH_BINDING)    uniform sampler2D depthImage;
layout(set = 0, binding = GBUFFER_GLOBAL_DIFFUSE_BINDING)  uniform sampler2D globalDiffuseImage;
#ifdef ENABLE_DIRECT_ILLUMINATION
layout(set = 0, binding = GBUFFER_DIRECT_DIFFUSE_BINDING)  uniform sampler2D directDiffuseImage;
layout(set = 0, binding = GBUFFER_SPECULAR_BINDING)        uniform sampler2D specularImage;
#endif
layout(set = 1, binding = DEBUG_SURFEL_BINDING)     uniform sampler2D debugImage;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 albedo  = texture(albedoImage, inUV);
    outColor = vec4(0.0);

    // global diffuse contribution
    if (control.data.showGlobalDiffuse != 0) {
        vec4 globalDiffuse = texture(globalDiffuseImage, inUV);
        globalDiffuse = clamp(globalDiffuse, vec4(0.0), vec4(1.0));
        outColor += globalDiffuse * albedo;
    }

    #ifdef ENABLE_DIRECT_ILLUMINATION
    if (control.data.showDirectDiffuse != 0) {
        // direct diffuse contribution
        vec4 directDiffuse = texture(directDiffuseImage, inUV);
        outColor += directDiffuse;

        // direct specular contribution
        vec4 directSpecular = texture(specularImage, inUV);
        outColor += directSpecular;
    }
    #endif

    // show debug points
    if (control.data.showSurfel != 0) {
        vec4 debug = texture(debugImage, inUV);
        outColor += debug;
    }

    /* #ifdef ENABLE_GAMMA_CORRECT */
    /* const float gamma = 1.0/2.2; */
    /* outColor.rgb = pow(outColor.rgb, vec3(gamma)); */
    /* #endif */
}
