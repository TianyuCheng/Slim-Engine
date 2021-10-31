#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform samplerCube skyboxMap;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

float signNotZero(float v) {
    return v >= 0.0 ? 1.0 : -1.0;
}

vec2 signNotZero(vec2 v) {
    return vec2(signNotZero(v.x), signNotZero(v.y));
}

// encode hemisphere into octahedron
vec2 encode_hemioct(vec3 v) {
    // project hemisphere onto hemi-octahedron
    vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + v.z));
    // rotate and scale the center diamon to the unit square
    return vec2(p.x + p.y, p.x - p.y);
}

// decode octahedron to hemisphere
vec3 decode_hemioct(vec2 e) {
    // rotate and scale the unit square back to the center diamond
    vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
    vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));
    return normalize(v);
}

// result in [-1, 1]^2
vec2 encode_oct(vec3 v) {
    vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
    // reflect the folds of the lower hemisphere over the diagonals
    return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
}

// result in [-1, +1]^3
vec3 decode_oct(vec2 e) {
    vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    return normalize(v);
}

void main() {
    vec3 uv3 = decode_oct(inUV * 2.0 - 1.0);
    outColor = texture(skyboxMap, uv3);

    #if 0
    vec3 uv3 = decode_hemioct(inUV * 2.0 - 1.0);
    outColor = texture(skyboxMap, uv3);
    #endif
}
