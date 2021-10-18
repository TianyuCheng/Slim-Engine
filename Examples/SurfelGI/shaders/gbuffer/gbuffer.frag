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

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out uint outInstanceID;

void main() {
    InstanceInfo instance = instances.data[object.instanceID];
    MaterialInfo material = materials.data[instance.materialID];

    uint baseColorTextureID = nonuniformEXT(material.baseColorTexture);
    uint baseColorSamplerID = nonuniformEXT(material.baseColorSampler);
    outAlbedo     = texture(sampler2D(textures[baseColorTextureID], samplers[baseColorSamplerID]), inUV);
    outNormal     = vec4(normalize(inNormal), 1.0);
    outInstanceID = object.instanceID;

    if (outAlbedo.a <= 0.25) {
        discard;
    }
}
