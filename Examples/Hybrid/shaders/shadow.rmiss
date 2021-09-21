#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) rayPayloadInEXT bool inShadow;

void main()
{
    inShadow = false;
}
