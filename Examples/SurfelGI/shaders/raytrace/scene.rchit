#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "../common.h"

// instances
layout(set = 0, binding = SCENE_INSTANCE_BINDING) buffer
Instance { InstanceInfo data[]; } instances;

// materials
layout(set = 0, binding = SCENE_MATERIAL_BINDING) buffer
Material { MaterialInfo data[]; } materials;

// scene textures
layout(set = 2, binding = SCENE_IMAGES_BINDING) uniform
texture2D textures[];

// scene samplers
layout(set = 3, binding = SCENE_SAMPLERS_BINDING) uniform
sampler samplers[];

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT HitInfo hitData;

layout(buffer_reference, scalar) buffer Indices   { ivec3        i[]; };
layout(buffer_reference, scalar) buffer Vertices  { VertexType   v[]; };
layout(buffer_reference, scalar) buffer Materials { MaterialInfo m[]; };

void main()
{
    InstanceInfo instance = instances.data[gl_InstanceCustomIndexEXT];

    Indices indices = Indices(instance.indexAddress);
    Vertices vertices = Vertices(instance.vertexAddress);
    Materials materials = Materials(instance.materialAddress);

    // indices
    ivec3 ind = indices.i[gl_PrimitiveID];

    // vertices
    VertexType v0 = vertices.v[ind.x];
    VertexType v1 = vertices.v[ind.y];
    VertexType v2 = vertices.v[ind.z];

    // material
    MaterialInfo m = materials.m[instance.materialID];

    // barycentrics
    vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // compute world position
    vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    // compute world normal at hit position
    vec3 normal = v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z;
    normal = normalize(instance.N * vec4(normal, 0.0)).xyz;
    if (gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT) {
        normal = -normal;
    }

    // compute texture coords
    vec2 uv0 = v0.uv0 * bary.x + v1.uv0 * bary.y + v2.uv0 * bary.z;

    // compute base color
    vec3 albedo = m.baseColor.xyz;
    int baseColorTextureID = nonuniformEXT(m.baseColorTexture);
    int baseColorSamplerID = nonuniformEXT(m.baseColorSampler);
    if (baseColorTextureID >= 0) {
        albedo *= texture(sampler2D(textures[baseColorTextureID],
                                    samplers[baseColorSamplerID]), uv0).xyz;
    }

    // compute base color
    vec3 emissive = m.emissiveColor.xyz;
    int emissiveTextureID = nonuniformEXT(m.emissiveTexture);
    int emissiveSamplerID = nonuniformEXT(m.emissiveSampler);
    if (emissiveTextureID >= 0) {
        emissive *= texture(sampler2D(textures[emissiveTextureID],
                                      samplers[emissiveSamplerID]), uv0).xyz;
    }

    // compute distance
    float dist = gl_HitTEXT;

    hitData.position = position;
    hitData.normal = normal;
    hitData.albedo = albedo;
    hitData.emissive = emissive;
    hitData.distance = dist;
}
