#version 450
#extension GL_ARB_separate_shader_objects : enable

// https://google.github.io/filament/Filament.md.html#materialsystem/diffusebrdf

const float sphericalHarmonics[9] = float[](
    // band l == 0
     0.28209479177387814,
    // band l == 1
    -0.4886025119029199,
     0.4886025119029199,
    -0.4886025119029199,
    // band l == 2
     1.0925484305920792,
    -1.0925484305920792,
     0.31539156525252005,
     1.0925484305920792,
    -1.0925484305920792
);

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
    int   occlusionTexCoord;

    float normalTexScale;
    int   normalTexCoord;
} materialFactors;
layout(set = 1, binding = 1) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessTexture;
layout(set = 1, binding = 3) uniform sampler2D normalTexture;
layout(set = 1, binding = 4) uniform sampler2D emissiveTexture;
layout(set = 1, binding = 5) uniform sampler2D occlusionTexture;

layout(set = 3, binding = 0) uniform sampler2D dfglutTexture;
layout(set = 3, binding = 1) uniform samplerCube environmentTexture;

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

const float PI = 3.1415926;

float Fd_Lambert() {
    return 1.0 / PI;
}

float IrradianceSH(vec3 n) {
    // We can use only the first 2 bands for better performance
    return sphericalHarmonics[0]
         + sphericalHarmonics[1] * (n.y)
         + sphericalHarmonics[2] * (n.z)
         + sphericalHarmonics[3] * (n.x)
         + sphericalHarmonics[4] * (n.y * n.x)
         + sphericalHarmonics[5] * (n.y * n.z)
         + sphericalHarmonics[6] * (3.0 * n.z * n.z - 1.0)
         + sphericalHarmonics[7] * (n.z * n.x)
         + sphericalHarmonics[8] * (n.x * n.x - n.y * n.y);
}

vec2 PrefilteredDFGLUT(float coord, float NoV) {
    // coord = sqrt(roughness) == perceptual roughness
    // IBL prefiltering code when computing the mipmaps
    return textureLod(dfglutTexture, vec2(NoV, coord), 0.0).rg;
}

vec3 EvaluateSpecularIBL(vec3 r, float perceptualRoughness) {
    // This assumes a 256x256 cubemap, with 9 mip levels
    float lod = 8.0 * perceptualRoughness;
    return textureLod(environmentTexture, r, lod).rgb;
}

vec3 EvaluateIBL(vec3 n, vec3 v, vec3 diffuseColor, vec3 f0, vec3 f90, float perceptualRoughness) {
    float NoV = max(dot(n, v), 0.0) + 1e-5;
    vec3 r = reflect(-v, n);

    // Specular indirect
    vec3 indirectSpecular = EvaluateSpecularIBL(r, perceptualRoughness);
    vec2 env = PrefilteredDFGLUT(perceptualRoughness, NoV);
    vec3 specularColor = f0 * env.x + f90 * env.y;

    // Diffuse indirect
    // We multiply by the Lambertian BRDF to compute radiance from irradiance with
    // the Disney BRDF we would have to remove the Fresnel term that depends on NoL.
    // The Lambertian BRDF can be baked directly in the SH to save a multiplication here.
    float indirectDiffuse = max(IrradianceSH(n), 0.0) * Fd_Lambert();

    // Indirect contribution
    return diffuseColor * indirectDiffuse + indirectSpecular * specularColor;
}

void main() {

    vec3 v = normalize(inView);
    vec3 n = normalize(inNormal);

    // retrieve metallic roughness
    vec2 metallicRoughness;
    metallicRoughness.x = materialFactors.metallicFactor;
    metallicRoughness.y = materialFactors.roughnessFactor;
    if (materialFactors.metallicRoughnessTexCoord == 0) {
        metallicRoughness = texture(metallicRoughnessTexture, inUV0).xy;
    } else if (materialFactors.metallicRoughnessTexCoord == 1) {
        metallicRoughness = texture(metallicRoughnessTexture, inUV1).xy;
    }
    float metallic = metallicRoughness.x;
    float perceptualRoughness = metallicRoughness.y;

    // // retrieve normal
    // if (materialFactors.normalTexCoord == 0) {
    //     texture(normalTexture, inUV0).xyz;
    // } else if (materialFactors.metallicRoughnessTexCoord == 1) {
    //     texture(normalTexture, inUV1).xyz;
    // }

    // ambient occlusion
    float occlusion = materialFactors.occlusionFactor;
    if (materialFactors.occlusionTexCoord == 0) {
        occlusion = texture(occlusionTexture, inUV0).x;
    } else if (materialFactors.occlusionTexCoord == 1) {
        occlusion = texture(occlusionTexture, inUV1).x;
    }

    // emissive
    vec3 emissive = materialFactors.emissiveFactor;
    if (materialFactors.emissiveTexCoord == 0) {
        emissive = texture(emissiveTexture, inUV0).xyz;
    } else if (materialFactors.emissiveTexCoord == 1) {
        emissive = texture(emissiveTexture, inUV1).xyz;
    }

    // base color
    vec4 baseColor = materialFactors.baseColorFactor;
    if (materialFactors.baseColorTexCoord == 0) {
        baseColor *= texture(baseColorTexture, inUV0);
    } else if (materialFactors.baseColorTexCoord == 1) {
        baseColor *= texture(baseColorTexture, inUV1);
    }

    // for dieletrics, base color is treated as reflectance
    // for conductors, base color is f0
    // linearly interpolate between dieletrics and conductors
    vec3 dieletricF0 = 0.16 * baseColor.rgb * baseColor.rgb * (1.0 - metallic);
    vec3 conductorF0 = baseColor.rgb * metallic;
    vec3 f0 = dieletricF0 + conductorF0;
    vec3 f90 = vec3(1.0);

    vec3 finalColor = EvaluateIBL(n, v, baseColor.rgb, f0, f90, perceptualRoughness) * occlusion + emissive;
    outColor = vec4(finalColor, baseColor.a);
}
