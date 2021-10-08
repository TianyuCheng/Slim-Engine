#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "raycommon.h"

layout(location = 0) rayPayloadInEXT ShadowHitPayload shadowHitPayload;

void main()
{
    shadowHitPayload.occlusionFactor = 1.0;   // not occluded
}
