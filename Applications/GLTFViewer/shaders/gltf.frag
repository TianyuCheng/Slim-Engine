#version 450
#extension GL_ARB_separate_shader_objects : enable

// material set
layout(set = 1, binding = 0) uniform MaterialFactors {
    vec4  baseColorFactor;
    int   baseColorTexCoord;

    float metallicFactor;
    float roughnessFactor;
    int   metallicRoughnessTexCoord;

    vec3  emissiveFactor;
    int   emissiveTexCoord;

    float occlusionFactor;
    int   occlusionTexture;
    int   occlusionTexCoord;

    float normalTexScale;
    int   normalTexCoord;
} materialFactors;
layout(set = 1, binding = 1) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessTexture;
layout(set = 1, binding = 3) uniform sampler2D normalTexture;
layout(set = 1, binding = 4) uniform sampler2D emissiveTexture;
layout(set = 1, binding = 5) uniform sampler2D occlusionTexture;

// varyings
layout(location = 0) in vec3 inView;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV0;
layout(location = 4) in vec2 inUV1;
layout(location = 5) in vec2 inColor0;
layout(location = 6) in vec2 inJoints0;
layout(location = 7) in vec2 inWeights0;

// output
layout(location = 0) out vec4 outColor;

// https://google.github.io/filament/Filament.md.html#materialsystem/diffusebrdf

const float PI = 3.1415926;

void main() {

    vec3 v = normalize(inView);
    vec3 n = normalize(inNormal);
    vec3 l = reflect(v, n);     // image-based light (IBL)
    vec3 h = normalize(v + l);  // l == n

    // Why do I need to add an epsilon?
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

    vec2 metallicRoughness;
    metallicRoughness.x = materialFactors.metallicFactor;
    metallicRoughness.y = materialFactors.roughnessFactor;

    // retrieve metallic roughness
    if (materialFactors.metallicRoughnessTexCoord == 0) {
        metallicRoughness = texture(metallicRoughnessTexture, inUV0).xy;
    } else if (materialFactors.metallicRoughnessTexCoord == 1) {
        metallicRoughness = texture(metallicRoughnessTexture, inUV1).xy;
    }

    // // retrieve normal
    // if (materialFactors.normalTexCoord == 0) {
    //     texture(normalTexture, inUV0).xyz;
    // } else if (materialFactors.metallicRoughnessTexCoord == 1) {
    //     texture(normalTexture, inUV1).xyz;
    // }

    // base color
    vec4 baseColor = materialFactors.baseColorFactor;
    if (materialFactors.baseColorTexCoord == 0) {
        baseColor *= texture(baseColorTexture, inUV0);
    } else if (materialFactors.baseColorTexCoord == 1) {
        baseColor *= texture(baseColorTexture, inUV1);
    }

    outColor = baseColor;
}
