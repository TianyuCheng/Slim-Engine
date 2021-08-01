#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable

// descriptor indexing
layout (set = 0, binding = 1) uniform sampler smp;
layout (set = 1, binding = 0) uniform texture2D textures[];

layout (location = 0) flat in int inMaterialId;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

void main() {
    int materialId = nonuniformEXT(inMaterialId);
    outColor = texture(sampler2D(textures[materialId], smp), inTexCoord);
}
