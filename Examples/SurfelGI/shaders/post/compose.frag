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
#ifdef ENABLE_SURFEL_GRID_VISUALIZATION
layout(set = 1, binding = DEBUG_GRID_BINDING)       uniform sampler2D gridImage;
#endif

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

// n must not be normalized (e.g. window coordinates)
float interleavedGradientNoise(highp vec2 n) {
    return fract(52.982919 * fract(dot(vec2(0.06711, 0.00584), n)));
}

vec4 ditherInterleavedGradientNoise(vec4 rgba, const highp float temporalNoise01) {
    // Jimenez 2014, "Next Generation Post-Processing in Call of Duty"
    highp vec2 uv = gl_FragCoord.xy + temporalNoise01;

    // The noise variable must be highp to workaround Adreno bug #1096.
    highp float noise = interleavedGradientNoise(uv);

    // remap from [0..1[ to [-0.5..0.5[
    noise -= 0.5;

    return rgba + vec4(noise / 255.0);
}

void main() {
    vec4 albedo  = texture(albedoImage, inUV);

    outColor.rgb = vec3(0.0);

    // global diffuse contribution
    if (control.data.showGlobalDiffuse != 0) {
        #if ENABLE_GI_POST_APPLY
        vec4 globalDiffuse = texture(globalDiffuseImage, inUV);
        outColor.rgb += globalDiffuse.rgb * albedo.rgb * globalDiffuse.a * albedo.a;
        #endif

        #if ENABLE_GI_PRE_APPLY
        vec4 directDiffuse = texture(globalDiffuseImage, inUV);
        outColor.rgb += directDiffuse.rgb * directDiffuse.a;
        #endif
    }

    #ifdef ENABLE_DIRECT_ILLUMINATION
    if (control.data.showDirectDiffuse != 0) {
        // direct diffuse contribution
        vec4 directDiffuse = texture(directDiffuseImage, inUV);
        outColor.rgb += directDiffuse.rgb * directDiffuse.a;

        // direct specular contribution
        vec4 directSpecular = texture(specularImage, inUV);
        outColor.rgb += directSpecular.rgb * directSpecular.a;
    }
    #endif

    outColor.a = 1.0;

    #ifdef ENABLE_OUTPUT_GAMMA_CORRECT
    const float gamma = 1.0/2.2;
    outColor.rgb = pow(outColor.rgb, vec3(gamma));
    #endif

    #ifdef ENABLE_OUTPUT_DITHER
    // dither
    outColor.rgba = ditherInterleavedGradientNoise(outColor, gl_FragCoord.x);
    #endif

    #ifdef ENABLE_OUTPUT_TONEMAP
    // tonemap
    outColor.rgb = ACESFilm(outColor.rgb);
    #endif

    #ifdef ENABLE_SURFEL_GRID_VISUALIZATION
    // show grid information
    if (control.data.showGrid != 0) {
        vec4 debug = texture(gridImage, inUV);
        outColor *= debug;
    }
    #endif

    // show surfel debug info
    if (control.data.showSurfelInfo != 0) {
        vec4 debug = texture(debugImage, inUV);
        outColor += debug;
    }

    if (control.data.showSurfelInfo == SURFEL_DEBUG_INCONSISTENCY) {
        vec4 debug = texture(debugImage, inUV);
        outColor = debug;
    }
}
