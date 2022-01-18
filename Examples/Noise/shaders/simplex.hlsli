// Reference: https://thebookofshaders.com/11/
// Reference: https://www.shadertoy.com/view/XsX3zB
// Reference: https://weber.itn.liu.se/~stegu/simplexnoise/simplexnoise.pdf

// For an N-d simplex noise, we need to smoothly interpolate O(2^N) points.
// As an improvement, simplex noise uses a equalateral triangle simplex.
// For an N-d simplex noise, we need to only evaluate O(N^2) points.

#ifndef SLIM_NOISE_LIB_SIMPLEX_NOISE
#define SLIM_NOISE_LIB_SIMPLEX_NOISE

#include "white.hlsli"

namespace noises {
namespace simplex {

    // get a scalar random value from a 2d value
    // for 1d simplex noise, there is no skew needed
    float rand1d(float value, float mutator = 0.546) {
        float x = value;

        // corner coordinates (nearest integer values)
        float i0 = floor(x);    // left corner
        float i1 = i0 + 1.0;    // right corner

        // distances to corners
        float x0 = x - i0;      // distance to left corner
        float x1 = i1 - x;      // distance to right corner

        // calculate surflet weights
        float2 w;
        w.x = dot(x0, x0);
        w.y = dot(x1, x1);

        // fade from 1.0 to 0.0 at the margin
        // raise to 4th power
        w = max(1.0 - w, 0.0);
        w = w * w;
        w = w * w;

        // calculate surflet components
        float2 d;
        d.x = dot(x0, noises::white::rand2d(i0, mutator));
        d.y = dot(x1, noises::white::rand2d(i1, mutator));

        return 0.395 * dot(w, d);
    }

    // get a scalar random value from a 2d value
    // for 2d simplex noise, we need to skew square grids into eauilateral triangles.
    float rand1d(float2 value, float2 mutator = float2(12.9898, 78.233)) {
        // skew constants for 2d simplex functions
        const float F2 = 0.366025403f;  // 0.5*(sqrt(3.0)-1.0)
        const float G2 = 0.211324865f;  // (3.0-sqrt(3.0))/6.0

        // find the first corner
        float2 s = floor(value + dot(value, F2));
        float2 x0 = value - s + dot(s, G2);

        // find other corners
        float2 i1 = (x0.x > x0.y) ? float2(1.0, 0.0) : float2(0.0, 1.0);
        float2 i2 = 1.0;
        float2 x1 = x0 - i1 + 1.0 * G2;
        float2 x2 = x0 - 1.0 + 2.0 * G2;

        // calculate surflet weights
        float3 w;
        w.x = dot(x0, x0);
        w.y = dot(x1, x1);
        w.z = dot(x2, x2);

        // fade from 0.5 to 0.0 at the margin
        // raise to 4th power
        w = max(0.5 - w, 0.0);
        w = w * w;
        w = w * w;

        // calculate surflet components
        float3 d;
        d.x = dot(x0, noises::white::rand2d(s,      mutator));
        d.y = dot(x1, noises::white::rand2d(s + i1, mutator));
        d.z = dot(x2, noises::white::rand2d(s + i2, mutator));

        return 45.23065 * dot(w, d);
    }

    // get a scalar random value from a 3d value
    // for 3d simplex noise, we need to skew square grids into tetrahedron
    float rand1d(float3 value, float3 mutator = float3(12.9898, 78.233, 37.719)) {
        const float F3 =  0.3333333;
        const float G3 =  0.1666667;

        // find the first corner
        float3 s = floor(value + dot(value, F3));
        float3 x0 = value - s + dot(s, G3);

        // find other corners
        float3 e = step(float3(0.0), x0 - x0.yzx);
        float3 i1 = e * (1.0 - e.zxy);
        float3 i2 = 1.0 - e.zxy*(1.0 - e);
        float3 i3 = 1.0;
        float3 x1 = x0 - i1 + G3;
        float3 x2 = x0 - i2 + 2.0 * G3;
        float3 x3 = x0 - 1.0 + 3.0 * G3;

        // calculate surflet weights
        float4 w;
        w.x = dot(x0, x0);
        w.y = dot(x1, x1);
        w.z = dot(x2, x2);
        w.w = dot(x3, x3);

        // fade from 0.5 to 0.0 at the margin
        // raise to 4th power
        w = max(0.5 - w, 0.0);
        w = w * w;
        w = w * w;

        // calculate surflet components
        float4 d;
        d.x = dot(x0, noises::white::rand2d(s,      mutator));
        d.y = dot(x1, noises::white::rand2d(s + i1, mutator));
        d.z = dot(x2, noises::white::rand2d(s + i2, mutator));
        d.w = dot(x3, noises::white::rand2d(s + i3, mutator));

        return 52.0 * dot(w, d);
    }

    // to 2d functions

    // get a 2d vector from an 1d value
    float2 rand2d(float value, float mutator = 0) {
        return float2(
            noises::simplex::rand1d(value + mutator, 3.9812),
            noises::simplex::rand1d(value + mutator, 7.1536)
        );
    }

    // get a 2d vector from an 2d value
    float2 rand2d(float2 value, float2 mutator = float2(0, 0)) {
        return float2(
            noises::simplex::rand1d(value + mutator, float2(12.989, 78.233)),
            noises::simplex::rand1d(value + mutator, float2(39.346, 11.135))
        );
    }

    // get a 2d vector from an 3d value
    float2 rand2d(float3 value, float3 mutator = float3(0, 0, 0)) {
        return float2(
            noises::simplex::rand1d(value + mutator, float3(12.989, 78.233, 37.719)),
            noises::simplex::rand1d(value + mutator, float3(39.346, 11.135, 83.155))
        );
    }

    //to 3d functions

    // get a 3d value from an 1d value
    float3 rand3d(float value, float mutator = 0) {
        return float3(
            noises::simplex::rand1d(value + mutator, 3.9812),
            noises::simplex::rand1d(value + mutator, 7.1536),
            noises::simplex::rand1d(value + mutator, 5.7241)
        );
    }

    // get a 3d value from a 2d value
    float3 rand3d(float2 value, float2 mutator = float2(0, 0)) {
        return float3(
            noises::simplex::rand1d(value + mutator, float2(12.989, 78.233)),
            noises::simplex::rand1d(value + mutator, float2(39.346, 11.135)),
            noises::simplex::rand1d(value + mutator, float2(73.156, 52.235))
        );
    }

    // get a 3d value from a 3d value
    float3 rand3d(float3 value, float3 mutator = float3(0, 0, 0)) {
        return float3(
            noises::simplex::rand1d(value + mutator, float3(12.989, 78.233, 37.719)),
            noises::simplex::rand1d(value + mutator, float3(39.346, 11.135, 83.155)),
            noises::simplex::rand1d(value + mutator, float3(73.156, 52.235, 09.151))
        );
    }

} // end of namespace simplex
} // end of namespace noises

#endif // SLIM_NOISE_LIB_SIMPLEX_NOISE
