#include <cmath>

inline float EV100(float aperture, float shutterSpeed, float sensitivity) noexcept {
    // With N = aperture, t = shutter speed and S = sensitivity,
    // we can compute EV100 knowing that:
    //
    // EVs = log2(N^2 / t)
    // and
    // EVs = EV100 + log2(S / 100)
    //
    // We can therefore find:
    //
    // EV100 = EVs - log2(S / 100)
    // EV100 = log2(N^2 / t) - log2(S / 100)
    // EV100 = log2((N^2 / t) * (100 / S))
    //
    // Reference: https://en.wikipedia.org/wiki/Exposure_value
    return std::log2((aperture * aperture) / shutterSpeed * 100.0f / sensitivity);
}

inline float Exposure(float ev100) noexcept {
    // The photometric exposure H is defined by:
    //
    // H = (q * t / (N^2)) * L
    //
    // Where t is the shutter speed, N the aperture, L the incident luminance
    // and q the lens and vignetting attenuation. A typical value of q is 0.65
    // (see reference link below).
    //
    // The value of H as recorded by a sensor depends on the sensitivity of the
    // sensor. An easy way to find that value is to use the saturation-based
    // sensitivity method:
    //
    // S_sat = 78 / H_sat
    //
    // This method defines the maximum possible exposure that does not lead to
    // clipping or blooming.
    //
    // The factor 78 is chosen so that exposure settings based on a standard
    // light meter and an 18% reflective surface will result in an image with
    // a grey level of 18% * sqrt(2) = 12.7% of saturation. The sqrt(2) factor
    // is used to account for an extra half a stop of headroom to deal with
    // specular reflections.
    //
    // Using the definitions of H and S_sat, we can derive the formula to
    // compute the maximum luminance to saturate the sensor:
    //
    // H_sat = 78 / S_stat
    // (q * t / (N^2)) * Lmax = 78 / S
    // Lmax = (78 / S) * (N^2 / (q * t))
    // Lmax = (78 / (S * q)) * (N^2 / t)
    //
    // With q = 0.65, S = 100 and EVs = log2(N^2 / t) (in this case EVs = EV100):
    //
    // Lmax = (78 / (100 * 0.65)) * 2^EV100
    // Lmax = 1.2 * 2^EV100
    //
    // The value of a pixel in the fragment shader can be computed by
    // normalizing the incident luminance L at the pixel's position
    // with the maximum luminance Lmax
    //
    // Reference: https://en.wikipedia.org/wiki/Film_speed
    return 1.0f / (1.2f * std::pow(2.0f, ev100));
}
