#ifndef SLIM_EXAMPLE_SHADERS_TRACING_H
#define SLIM_EXAMPLE_SHADERS_TRACING_H
#ifndef __cplusplus

bool trace_scene(vec3 rayOrigin, vec3 rayDirection, float tMin, float tMax) {
    traceRayEXT(sceneAS,          // acceleration structure
            gl_RayFlagsOpaqueEXT, // rayFlags
            0xff,                 // cullMask
            0,                    // sbtRecordOffset
            0,                    // sbtRecordStride
            0,                    // missIndex
            rayOrigin,            // ray origin
            tMin,                 // ray min range
            rayDirection,         // ray direction
            tMax,                 // ray max range
            0);                   // payload (location = 0)
    return hitData.distance > 0.0;
}

bool trace_shadow(vec3 rayOrigin, vec3 rayDirection, float tMin, float tMax) {
    const uint flags = gl_RayFlagsTerminateOnFirstHitEXT
                     | gl_RayFlagsSkipClosestHitShaderEXT
                     | gl_RayFlagsOpaqueEXT;
    shadowed = true;
    traceRayEXT(sceneAS,          // acceleration structure
            flags,                // rayFlags
            0xff,                 // cullMask
            0,                    // sbtRecordOffset
            0,                    // sbtRecordStride
            1,                    // missIndex
            rayOrigin,            // ray origin
            tMin,                 // ray min range
            rayDirection,         // ray direction
            tMax,                 // ray max range
            1);                   // payload (location = 1)
    return shadowed;
}

#endif
#endif // SLIM_EXAMPLE_SHADERS_TRACING_H
