#version 450
#extension GL_ARB_separate_shader_objects : enable

// https://google.github.io/filament/Filament.md.html

// material set
layout(set = 1, binding = 0) uniform MaterialFactors {
    vec4      baseColor;
    vec4      emissive;

    // metalness, roughness, occlusion, normal scale
    float     metalness;
    float     roughness;
    float     occlusion;
    float     normalScale;

    // texture coord set
    int       baseColorTexCoordSet;
    int       emissiveTexCoordSet;
    int       occlusionTexCoordSet;
    int       normalTexCoordSet;
    int       metallicRoughnessTexCoordSet;

    // texture info (reserved for descriptor indexing)
    int       baseColorTexture;
    int       emissiveTexture;
    int       occlusionTexture;
    int       normalTexture;
    int       metallicRoughnessTexture;

    // sampler info (reserved for descriptor indexing)
    int       baseColorSampler;
    int       emissiveSampler;
    int       occlusionSampler;
    int       normalSampler;
    int       metallicRoughnessSampler;

    // other information
    int       alphaMode;
} materialFactors;
layout(set = 1, binding = 1) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessTexture;
layout(set = 1, binding = 3) uniform sampler2D normalTexture;
layout(set = 1, binding = 4) uniform sampler2D emissiveTexture;
layout(set = 1, binding = 5) uniform sampler2D occlusionTexture;

layout(set = 3, binding = 0) uniform sampler2D dfglutTexture;
layout(set = 3, binding = 1) uniform samplerCube environmentTexture;
layout(set = 3, binding = 2) uniform samplerCube irradianceTexture;

// varyings
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inView;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec2 inUV0;
layout(location = 5) in vec2 inUV1;
layout(location = 6) in vec2 inColor0;
layout(location = 7) in vec2 inJoints0;
layout(location = 8) in vec2 inWeights0;

// output
layout(location = 0) out vec4 outColor;

const float PI = 3.1415926;

float Fd_Lambert() {
    // Lambertian BRDF reflects uniformly in the hemisphere
    return 1.0 / PI;
}

vec2 PrefilteredDFGLUT(float coord, float NoV) {
    // coord = sqrt(roughness) == perceptual roughness
    // IBL prefiltering code when computing the mipmaps
    return textureLod(dfglutTexture, vec2(NoV, coord), 0.0).rg;
}

vec3 EvaluateSpecularIBL(vec3 r, float perceptualRoughness) {
    // This assumes N mip levels, wheen N == 12, it means 4k image
    float lod = 12.0 * perceptualRoughness;
    return textureLod(environmentTexture, r, lod).rgb;
}

vec3 EvaluateDiffuseIBL(vec3 n) {
    // It is possible to replace this texture look up with a 3-band (9 coefficients)
    // spherical harmonics (very little perceptual difference), but I am lazy now.

    // We multiply by the Lambertian BRDF to compute radiance from irradiance.
    // With the Disney BRDF we would have to remove the Fresnel term that depends on NoL.
    // The Lambertian BRDF can be baked directly in the SH to save a multiplication here.
    return texture(irradianceTexture, n).rgb * Fd_Lambert();
}

vec3 EvaluateIBL(vec3 n, vec3 v, vec3 diffuseColor, vec3 f0, vec3 f90, float perceptualRoughness) {
    float NoV = max(dot(n, v), 0.0);
    vec3 r = reflect(-v, n);

    // Specular indirect
    vec3 indirectSpecular = EvaluateSpecularIBL(r, perceptualRoughness);
    vec2 env = PrefilteredDFGLUT(perceptualRoughness, NoV);
    vec3 specularColor = f0 * env.x + f90 * env.y;

    // Diffuse indirect
    vec3 indirectDiffuse = EvaluateDiffuseIBL(n);

    // Indirect contribution
    return indirectDiffuse * diffuseColor + indirectSpecular * specularColor;
}

mat3 ComputeTBN(vec3 normal, vec3 position, vec2 texcoord) {
    // Compute derivative of the world position
    vec3 posDx = dFdx(position);
    vec3 posDy = dFdy(position);

    // Compute derivative of the texture coordinate
    vec2 tcDx = dFdx(texcoord);
    vec2 tcDy = dFdy(texcoord);

    // Compute initial tangent and bi-tangent
    vec3 t = normalize(tcDy.y * posDx - tcDx.y * posDy);
    vec3 b = normalize(tcDy.x * posDx - tcDx.x * posDy);
    vec3 n = normalize(normal);

    vec3 x;

    x = cross(n, t);
    t = normalize(cross(x, n));

    x = cross(b, n);
    b = normalize(cross(n, x));

    return mat3(t, b, n);
}

void main() {

    vec3 v = normalize(-inView);
    vec3 n = normalize(inNormal);

    // retrieve metallic roughness
    vec2 metallicRoughness;
    metallicRoughness.x = materialFactors.metalness;
    metallicRoughness.y = materialFactors.roughness;
    if (materialFactors.metallicRoughnessTexCoordSet == 0) {
        metallicRoughness = texture(metallicRoughnessTexture, inUV0).xy;
    } else if (materialFactors.metallicRoughnessTexCoordSet == 1) {
        metallicRoughness = texture(metallicRoughnessTexture, inUV1).xy;
    }
    float metallic = metallicRoughness.x;
    float perceptualRoughness = metallicRoughness.y;

    // retrieve normal
    if (materialFactors.normalTexCoordSet == 0) {
        vec3 perturb = texture(normalTexture, inUV0).xyz * 2.0 - 1.0;
        mat3 tbn = ComputeTBN(inNormal, inPosition, inUV0);
        n = normalize(n + tbn * perturb * materialFactors.normalScale);
    } else if (materialFactors.normalTexCoordSet == 1) {
        vec3 perturb = texture(normalTexture, inUV1).xyz * 2.0 - 1.0;
        mat3 tbn = ComputeTBN(inNormal, inPosition, inUV1);
        n = normalize(n + tbn * perturb * materialFactors.normalScale);
    }

    // ambient occlusion
    float occlusion = materialFactors.occlusion;
    if (materialFactors.occlusionTexCoordSet == 0) {
        occlusion = texture(occlusionTexture, inUV0).x;
    } else if (materialFactors.occlusionTexCoordSet == 1) {
        occlusion = texture(occlusionTexture, inUV1).x;
    }

    // emissive
    vec3 emissive = materialFactors.emissive.xyz;
    if (materialFactors.emissiveTexCoordSet == 0) {
        emissive = texture(emissiveTexture, inUV0).xyz;
    } else if (materialFactors.emissiveTexCoordSet == 1) {
        emissive = texture(emissiveTexture, inUV1).xyz;
    }

    // base color
    vec4 baseColor = materialFactors.baseColor;
    if (materialFactors.baseColorTexCoordSet == 0) {
        baseColor *= texture(baseColorTexture, inUV0);
    } else if (materialFactors.baseColorTexCoordSet == 1) {
        baseColor *= texture(baseColorTexture, inUV1);
    }

    // for dielectrics, base color is treated as reflectance
    // for conductors, base color is f0
    // linearly interpolate between dielectrics and conductors
    vec3 dielectricF0 = vec3(0.04);
    vec3 conductorF0 = baseColor.rgb;
    vec3 f0 = mix(dielectricF0, conductorF0, metallic);
    vec3 f90 = vec3(1.0);
    vec3 finalColor = EvaluateIBL(n, v, baseColor.rgb, f0, f90, perceptualRoughness) * occlusion + emissive;
    outColor = vec4(finalColor, baseColor.a);
}
