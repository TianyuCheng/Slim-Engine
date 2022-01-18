// https://www.ronja-tutorials.com/post/026-perlin-noise/
// with functions renamed and re-organized

// Similar to value noise, perlin noise is also a smooth noise.
// It is an implementation of so-called "gradient noise".
// What differentiates it from value noise is that instead of
// interpolating the values, the values are based on inclinations.

#ifndef SLIM_NOISE_LIB_PERLIN_NOISE
#define SLIM_NOISE_LIB_PERLIN_NOISE

#include "white.hlsli"

namespace noises {
namespace perlin {

    float fade(float t) {
        #if 0
        // Cubic Hermite Curve. Same as SmoothStep()
        return t * t * (3.0 - 2.0 * t);
        #else
        // Quintic interpolation curve
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
        #endif
    }

    float2 fade(float2 t) {
        return float2(
                noises::perlin::fade(t.x),
                noises::perlin::fade(t.y));
    }

    float3 fade(float3 t) {
        return float3(
                noises::perlin::fade(t.x),
                noises::perlin::fade(t.y),
                noises::perlin::fade(t.z));
    }

    // to 1d functions

    float rand1d(float value, float mutator) {
        // generate random directions
        float dirL     = noises::white::rand1d(floor(value), mutator);
        float dirR     = noises::white::rand1d( ceil(value), mutator);
        // get values of cell based on fraction and cell directions
        float fraction = frac(value);
        float t        = noises::perlin::fade(fraction);
        float cellL    = dirL * t;
        float cellR    = dirR * (t - 1);
        // perform interpolation
        float cell     = lerp(cellL, cellR, fraction);
        return cell;
    }

    float rand1d(float2 value, float2 mutator = float2(12.9898, 78.233)) {
        // generate random directions
        float2 dirBL    = noises::white::rand2d(float2(floor(value.x), floor(value.y)), mutator);
        float2 dirBR    = noises::white::rand2d(float2( ceil(value.x), floor(value.y)), mutator);
        float2 dirTL    = noises::white::rand2d(float2(floor(value.x),  ceil(value.y)), mutator);
        float2 dirTR    = noises::white::rand2d(float2( ceil(value.x),  ceil(value.y)), mutator);
        // get values of cell based on fraction and cell directions
        float2 fraction = frac(value);
        float  cellBL   = dot(dirBL, fraction - float2(0, 0));
        float  cellBR   = dot(dirBR, fraction - float2(1, 0));
        float  cellTL   = dot(dirTL, fraction - float2(0, 1));
        float  cellTR   = dot(dirTR, fraction - float2(1, 1));
        // perform interpolation
        float2 t        = noises::perlin::fade(fraction);
        float  cellB    = lerp(cellBL, cellBR, t.x);
        float  cellT    = lerp(cellTL, cellTR, t.x);
        float  cell     = lerp(cellB, cellT, t.y);
        return cell;
    }

    float rand1d(float3 value, float3 mutator = float3(12.9898, 78.233, 37.719)) {
        // generate random directions
        float3 dirBTL   = noises::white::rand3d(float3(floor(value.x),  ceil(value.y), floor(value.z)), mutator);
        float3 dirBTR   = noises::white::rand3d(float3( ceil(value.x),  ceil(value.y), floor(value.z)), mutator);
        float3 dirFTL   = noises::white::rand3d(float3(floor(value.x),  ceil(value.y),  ceil(value.z)), mutator);
        float3 dirFTR   = noises::white::rand3d(float3( ceil(value.x),  ceil(value.y),  ceil(value.z)), mutator);
        float3 dirBBL   = noises::white::rand3d(float3(floor(value.x), floor(value.y), floor(value.z)), mutator);
        float3 dirBBR   = noises::white::rand3d(float3( ceil(value.x), floor(value.y), floor(value.z)), mutator);
        float3 dirFBL   = noises::white::rand3d(float3(floor(value.x), floor(value.y),  ceil(value.z)), mutator);
        float3 dirFBR   = noises::white::rand3d(float3( ceil(value.x), floor(value.y),  ceil(value.z)), mutator);
        // get values of cell based on fraction and cell directions
        float3 fraction = frac(value);
        float  cellBTL  = dot(dirBTL, fraction - float3(0, 1, 0));
        float  cellBTR  = dot(dirBTR, fraction - float3(1, 1, 0));
        float  cellFTL  = dot(dirFTL, fraction - float3(0, 1, 1));
        float  cellFTR  = dot(dirFTR, fraction - float3(1, 1, 1));
        float  cellBBL  = dot(dirBBL, fraction - float3(0, 0, 0));
        float  cellBBR  = dot(dirBBR, fraction - float3(1, 0, 0));
        float  cellFBL  = dot(dirFBL, fraction - float3(0, 0, 1));
        float  cellFBR  = dot(dirFBR, fraction - float3(1, 0, 1));
        // perform interpolation
        float3 t        = noises::perlin::fade(fraction);
        float  cellBT   = lerp(cellBTL, cellBTR, t.x);
        float  cellBB   = lerp(cellBBL, cellBBR, t.x);
        float  cellFT   = lerp(cellFTL, cellFTR, t.x);
        float  cellFB   = lerp(cellFBL, cellFBR, t.x);
        float  cellB    = lerp(cellBB, cellBT, t.y);
        float  cellF    = lerp(cellFB, cellFT, t.y);
        float  cell     = lerp(cellB, cellF, t.z);
        return cell;
    }

    // to 2d functions

    // get a 2d vector from an 1d value
    float2 rand2d(float value, float mutator = 0) {
        return float2(
            noises::perlin::rand1d(value + mutator, 3.9812),
            noises::perlin::rand1d(value + mutator, 7.1536)
        );
    }

    // get a 2d vector from an 2d value
    float2 rand2d(float2 value, float2 mutator = float2(0, 0)) {
        return float2(
            noises::perlin::rand1d(value + mutator, float2(12.989, 78.233)),
            noises::perlin::rand1d(value + mutator, float2(39.346, 11.135))
        );
    }

    // get a 2d vector from an 3d value
    float2 rand2d(float3 value, float3 mutator = float3(0, 0, 0)) {
        return float2(
            noises::perlin::rand1d(value + mutator, float3(12.989, 78.233, 37.719)),
            noises::perlin::rand1d(value + mutator, float3(39.346, 11.135, 83.155))
        );
    }

    //to 3d functions

    // get a 3d value from an 1d value
    float3 rand3d(float value, float mutator = 0) {
        return float3(
            noises::perlin::rand1d(value + mutator, 3.9812),
            noises::perlin::rand1d(value + mutator, 7.1536),
            noises::perlin::rand1d(value + mutator, 5.7241)
        );
    }

    // get a 3d value from a 2d value
    float3 rand3d(float2 value, float2 mutator = float2(0, 0)) {
        return float3(
            noises::perlin::rand1d(value + mutator, float2(12.989, 78.233)),
            noises::perlin::rand1d(value + mutator, float2(39.346, 11.135)),
            noises::perlin::rand1d(value + mutator, float2(73.156, 52.235))
        );
    }

    // get a 3d value from a 3d value
    float3 rand3d(float3 value, float3 mutator = float3(0, 0, 0)) {
        return float3(
            noises::perlin::rand1d(value + mutator, float3(12.989, 78.233, 37.719)),
            noises::perlin::rand1d(value + mutator, float3(39.346, 11.135, 83.155)),
            noises::perlin::rand1d(value + mutator, float3(73.156, 52.235, 09.151))
        );
    }

} // end of namespace perlin
} // end of namespace noises

#endif // end of SLIM_NOISE_LIB_PERLIN_NOISE
