#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

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

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
