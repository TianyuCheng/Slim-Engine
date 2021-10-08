#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "surfel.h"
#include "raycommon.h"

hitAttributeEXT vec2 attribs;

// surfel information
layout(set = 1, binding = 0) buffer SurfelStatBuffer { SurfelStat data;   } surfelStat;
layout(set = 1, binding = 1) buffer SurfelDataBuffer { SurfelData data[]; } surfelData;
layout(set = 1, binding = 2) buffer SurfelBuffer     { Surfel     data[]; } surfels;

layout(location = 1) rayPayloadInEXT SurfelHitPayload hitPayload;

void main()
{
    /* uint surfel_index = gl_HitKindEXT; */
    /*  */
    /* vec3 rayOrigin = gl_WorldRayOriginEXT; */
    /* vec3 rayDirection = gl_WorldRayDirectionEXT; */
    /* vec3 hitPos = rayOrigin + gl_HitTEXT * rayDirection; */
    /*  */
    /* SurfelData surfelData = surfelData.data[surfel_index]; */
    /* surfelData.hitPos = hitPos; */
    /*  */
    /* Surfel surfel = surfels.data[surfel_index]; */
    /* hitPayload.color = surfel.color; */

    hitPayload.color = vec3(0.0, 1.0, 0.0);
}
