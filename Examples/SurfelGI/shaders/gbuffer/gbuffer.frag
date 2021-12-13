#version 450
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "../common.h"

layout(push_constant) uniform Object { uint instanceID; } object;

// Instances
layout(set = 0, binding = SCENE_INSTANCE_BINDING) buffer
Instance { InstanceInfo data[]; } instances;

// Materials
layout(set = 0, binding = SCENE_MATERIAL_BINDING) buffer
Material { MaterialInfo data[]; } materials;

// images
layout(set = 1, binding = SCENE_IMAGES_BINDING)   uniform texture2D textures[];

// samplers
layout(set = 2, binding = SCENE_SAMPLERS_BINDING) uniform sampler   samplers[];

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;
#ifdef ENABLE_GBUFFER_WORLD_POSITION
layout(location = 2) in vec3 inPosition;
#endif

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outEmissive;
layout(location = 2) out vec4 outMetallicRoughness;
layout(location = 3) out vec4 outNormal;
layout(location = 4) out uint outInstanceID;
#ifdef ENABLE_GBUFFER_WORLD_POSITION
layout(location = 5) out vec4 outPosition;
#endif

void main() {
    const float gamma = 2.2;

    InstanceInfo instance = instances.data[object.instanceID];
    MaterialInfo material = materials.data[instance.materialID];

    // base color
    int baseColorTextureID = nonuniformEXT(material.baseColorTexture);
    int baseColorSamplerID = nonuniformEXT(material.baseColorSampler);
    if (baseColorTextureID >= 0) {
        outAlbedo = texture(sampler2D(textures[baseColorTextureID],
                                      samplers[baseColorSamplerID]), inUV);
        #ifdef ENABLE_INPUT_GAMMA_CORRECT
        outAlbedo.rgb = pow(outAlbedo.rgb, vec3(gamma));
        #endif
    } else {
        outAlbedo = material.baseColor;
    }

    // emissive
    int emissiveTextureID = nonuniformEXT(material.emissiveTexture);
    int emissiveSamplerID = nonuniformEXT(material.emissiveSampler);
    if (emissiveTextureID >= 0) {
        outEmissive = texture(sampler2D(textures[emissiveTextureID],
                                        samplers[emissiveSamplerID]), inUV);
        #ifdef ENABLE_INPUT_GAMMA_CORRECT
        outEmissive.rgb = pow(outEmissive.rgb, vec3(gamma));
        #endif
    } else {
        outEmissive = material.emissiveColor;
    }

    // metallic roughness
    int metallicRoughnessTextureID = nonuniformEXT(material.metallicRoughnessTexture);
    int metallicRoughnessSamplerID = nonuniformEXT(material.metallicRoughnessSampler);
    if (metallicRoughnessTextureID >= 0) {
        outMetallicRoughness = texture(sampler2D(textures[metallicRoughnessTextureID],
                                                 samplers[metallicRoughnessSamplerID]), inUV).bgra;
    } else {
        outMetallicRoughness = vec4(
            material.metalness,
            material.roughness,
            0.0, 1.0);
    }

    // others
    outNormal = vec4(normalize(inNormal), 1.0);
    outInstanceID = object.instanceID;

    #ifdef ENABLE_GBUFFER_WORLD_POSITION
    outPosition   = vec4(inPosition, 1.0);
    #endif

    if (outAlbedo.a <= 0.25) {
        discard;
    }
}
