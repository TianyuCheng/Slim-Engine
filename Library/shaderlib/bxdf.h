#ifndef SLIM_SHADER_LIB_BXDF_H
#define SLIM_SHADER_LIB_BXDF_H

#include "glsl.hpp"

// reference:
// https://google.github.io/filament/Filament.md.html

// normal distribution function (specular D)
// observation: long-tailed normal distribution are good fit for real world surfaces.

SLIM_ATTR float D_GGX(float NoH, float roughness) {
    float a = NoH * roughness;
    float k = roughness / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / PI);
}

// This is an improvement to the original GGX by computing 1 - pow(dot(n, h), 2) in half.
// This suffers from math cancellation when pow(dot(n, h), 2) is close to 1.
// So we compute pow(1 - cross(a, b), 2) instead.
SLIM_ATTR float D_GGX(float NoH, float roughness, const vec3 n, const vec3 h) {
    vec3 NxH = cross(n, h);
    float a = NoH * roughness;
    float k = roughness / (dot(NxH, NxH) + a * a);
    float d = k * k * (1.0 / PI);
    return saturate_mediump(d);
}

// geometry shadowing (specular G)

SLIM_ATTR float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

// This approximation is not mathematically coorrect, but fast
SLIM_ATTR float V_SmithGGXCorrelatedFast(float NoV, float NoL, float roughness) {
    float a = roughness;
    float GGXV = NoL * (NoV * (1.0 - a) + a);
    float GGXL = NoV * (NoL * (1.0 - a) + a);
    return 0.5 / (GGXV + GGXL);
}

// Fresnel (specular F)

// setting f90 = 1
SLIM_ATTR vec3 F_Schlick(float VoH, vec3 f0) {
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0f - f);
}

SLIM_ATTR float F_Schlick(float VoH, float f0) {
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0f - f);
}

SLIM_ATTR vec3 F_Schlick(float VoH, vec3 f0, vec3 f90) {
    return f0 + (f90 - f0) * pow(1.0f - VoH, 5.0f);
}

SLIM_ATTR float F_Schlick(float VoH, float f0, float f90) {
    return f0 + (f90 - f0) * pow(1.0f - VoH, 5.0f);
}

// Diffuse BRDF

SLIM_ATTR float Fd_Lambert() {
    return 1.0 / PI;
}

SLIM_ATTR float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(NoL, 1.0, f90);
    float viewScatter = F_Schlick(NoV, 1.0, f90);
    return lightScatter * viewScatter * (1.0 / PI);
}

#endif // SLIM_SHADER_LIB_BXDF_H
