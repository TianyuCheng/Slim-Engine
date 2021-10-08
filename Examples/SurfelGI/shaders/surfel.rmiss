#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "raycommon.h"

layout(location = 1) rayPayloadInEXT SurfelHitPayload hitPayload;

void main()
{
    /* hitPayload.color = vec3(0.0, 0.0, 0.0); */
    hitPayload.color = vec3(1.0, 0.0, 0.0);
}
