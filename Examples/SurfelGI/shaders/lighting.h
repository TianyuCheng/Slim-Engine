#ifndef SLIM_EXAMPLE_SHADERS_LIGHTING_H
#define SLIM_EXAMPLE_SHADERS_LIGHTING_H
#ifndef __cplusplus

vec3 compute_directional_lighting(in LightInfo light,
                                  in HitInfo surface,
                                  out float NoL,
                                  out float dist,
                                  out vec3 lightDirection) {
    dist = HIGHP_FLT_MAX;
    lightDirection = -normalize(light.direction);
    NoL = dot(surface.normal, lightDirection);
    return light.color * light.intensity * max(0.0, NoL);
}

vec3 compute_directional_lighting(in LightInfo light,
                                  in HitInfo surface,
                                  out float NoL,
                                  out float dist) {
    vec3 lightDirection;
    return compute_directional_lighting(light, surface, NoL, dist, lightDirection);
}

vec3 compute_point_lighting(in LightInfo light,
                            in HitInfo surface,
                            out float NoL,
                            out float dist,
                            out vec3 lightDirection) {
    lightDirection = light.position - surface.position;
    float d2 = dot(lightDirection, lightDirection);
    float r2 = light.range * light.range;
    // check maximum range
    if (d2 < r2) {
        dist = sqrt(d2);
        lightDirection /= dist;
        // compute attenuation if needed
        NoL = dot(surface.normal, lightDirection);
        if (NoL > 0.0) {
            float n = 2;
            float att1 = clamp(1.0 - (pow(dist, n) / pow(light.range, n)), 0.0, 1.0);
            float att2 = 1.0 / d2;
            return light.color * light.intensity * att1 * att2;
        }
    }
    return vec3(0.0);
}

vec3 compute_point_lighting(in LightInfo light,
                            in HitInfo surface,
                            out float NoL,
                            out float dist) {
    vec3 lightDirection;
    return compute_point_lighting(light, surface, NoL, dist, lightDirection);
}

vec3 compute_spot_lighting(in LightInfo light,
                           in HitInfo surface,
                           out float NoL,
                           out float dist,
                           out vec3 lightDirection) {
    lightDirection = light.position - surface.position;
    float d2 = dot(lightDirection, lightDirection);
    float r2 = light.range * light.range;
    // check maximum range
    if (d2 < r2) {
        dist = sqrt(d2);
        lightDirection /= dist;
        // compute attenuation if needed
        NoL = dot(surface.normal, lightDirection);
        if (NoL > 0.0) {
            float spotFactor = dot(lightDirection, light.direction);
            float spotCutOff = cos(light.angle);
            if (spotFactor > spotCutOff) {
                float att1 = clamp(1.0 - (d2 / r2), 0.0, 1.0);
                float att2 = att1 * att1;
                float attenuation = clamp(1.0 - (1.0 - spotFactor) * 1.0 / (1.0 - spotCutOff), 0.0, 1.0);
                return light.color * light.intensity * att2;
            }
        }
    }
    return vec3(0.0);
}

vec3 compute_spot_lighting(in LightInfo light,
                           in HitInfo surface,
                           out float NoL,
                           out float dist) {
    vec3 lightDirection;
    return compute_spot_lighting(light, surface, NoL, dist, lightDirection);
}

vec2 linear_sample_radial_depth(vec2 pixel) {
    int2 base = int2(pixel / vec2(SURFEL_DEPTH_TEXELS)) * int2(SURFEL_DEPTH_TEXELS);
    vec2 tile = pixel - vec2(base) + vec2(0.5);
    int2 co = int2(tile);
    vec2 uv = fract(tile);

    int2 c0 = base + max(co - int2(1), int2(0));
    int2 c1 = base + min(co, int2(SURFEL_DEPTH_TEXELS - 1));
    vec4 p00 = imageLoad(surfelDepth, int2(c0.x, c0.y));
    vec4 p10 = imageLoad(surfelDepth, int2(c1.x, c0.y));
    vec4 p01 = imageLoad(surfelDepth, int2(c0.x, c1.y));
    vec4 p11 = imageLoad(surfelDepth, int2(c1.x, c1.y));
    return mix(mix(p00, p10, uv.x), mix(p01, p11, uv.x), uv.y).xy;
}

vec3 compute_surfel_lighting(in CameraInfo camera, in vec3 P, in vec3 N) {
    vec4 surfelGI = vec4(0.0);

    // fetch surfel grid
    int3 gridIndex = compute_surfel_grid(camera, P);
    uint cellIndex = compute_surfel_cell(gridIndex);
    SurfelGridCell cell = surfelGrids.data[cellIndex];

    // iterate through all surfels in this grid cell
    /* uint next = cell.offset + min(cell.count, 128); */
    uint next = cell.offset + cell.count;
    for (uint i = cell.offset; i < next; i++) {
        uint cellIndex = surfelCells.data[i];
        Surfel surfel = surfels.data[cellIndex];

        // apply surfel coverage
        vec3 L = surfel.position - P;
        float d2 = dot(L, L);
        float r2 = surfel.radius * surfel.radius;

        if (d2 < r2) {
            vec3 surfelN = unpack_snorm3(surfel.normal);
            float dotN = dot(surfelN, N);
            if (dotN > 0.0) {
#if 0
                float dist = sqrt(d2);
                float contribution = 1.0;
                contribution *= clamp(dotN, 0.0, 1.0);
                contribution *= clamp(1.0 - dist / surfel.radius, 0.0, 1.0);

                #ifdef ENABLE_SURFEL_RADIAL_DEPTH
                // radial depth function
                vec2 radialDepthPixel = compute_surfel_depth_pixel(cellIndex, -L/dist, surfelN);
                vec2 radialDepth = linear_sample_radial_depth(radialDepthPixel);
                contribution *= compute_surfel_depth_weight(radialDepth, dist);
                #endif

                // surfel moment helps prevent incorrect color bleeding.
                contribution = smoothstep(0.0, 1.0, contribution);
                surfelGI += vec4(surfel.color, 1.0) * contribution;
#else
                float dist = sqrt(d2);
                float occlusion = 1.0;
                #ifdef ENABLE_SURFEL_RADIAL_DEPTH
                // radial depth function
                vec2 radialDepthPixel = compute_surfel_depth_pixel(cellIndex, -L/dist, surfelN);
                vec2 radialDepth = linear_sample_radial_depth(radialDepthPixel);
                occlusion *= compute_surfel_depth_weight(radialDepth, dist);
                #endif

                const float RADIUS_OVERSCALE = 1.25;
                const float SURFEL_NORMAL_DIRECTION_SQUISH = 2.0;
                const vec3 pos_offset = P - surfel.position;
                const float mahalanobis_dist = length(pos_offset) * (1 + abs(dot(pos_offset, surfelN)) * SURFEL_NORMAL_DIRECTION_SQUISH);
                float contribution = smoothstep(
                    surfel.radius * RADIUS_OVERSCALE,
                    0.0, mahalanobis_dist) * dotN * occlusion;
                surfelGI += vec4(surfel.color, 1.0) * contribution;
#endif
            }
        }
    }
    if (surfelGI.a > 0.0) {
        surfelGI.rgb /= surfelGI.a;
        surfelGI.a = clamp(surfelGI.a, 0.0, 1.0);
        surfelGI.rgb *= surfelGI.a;
    }
    return surfelGI.rgb;
}

#endif
#endif // SLIM_EXAMPLE_SHADERS_LIGHTING_H
