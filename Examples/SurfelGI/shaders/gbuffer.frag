#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Object {
    uint instanceID;
    uint baseColorTextureID;
    uint baseColorSamplerID;
} object;

layout (set = 1, binding = 0) uniform texture2D textures[];
layout (set = 2, binding = 0) uniform sampler   samplers[];

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outWorldPos;
layout(location = 2) out vec4 outWorldNormal;
layout(location = 3) out uint outInstanceID;

void main() {
    uint baseColorTextureId = nonuniformEXT(object.baseColorTextureID);
    uint baseColorSamplerId = nonuniformEXT(object.baseColorSamplerID);
    outAlbedo       = texture(sampler2D(textures[baseColorTextureId], samplers[baseColorSamplerId]), inUV);
    outWorldPos     = vec4(inWorldPos, 1.0);
    outWorldNormal  = vec4(normalize(inWorldNormal), 1.0);
    outInstanceID   = object.instanceID;
}
