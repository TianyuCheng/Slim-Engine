#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inAlbedo;
layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inNormal;
layout (input_attachment_index = 1, set = 1, binding = 2) uniform subpassInput inPosition;

layout(location = 0) in vec3 inFairyPosition;
layout(location = 1) in vec3 inFairyColor;
layout(location = 2) in float inFairyRadius;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo = subpassLoad(inAlbedo).rgb;
    vec3 normal = subpassLoad(inNormal).rgb;
    vec3 position = subpassLoad(inPosition).rgb;

    float dist = distance(inFairyPosition, position);
    float atten = 1.0 / (1.0 + 0.1*dist + 0.01*dist*dist);

    vec3 L = normalize(inFairyPosition - position);
    vec3 N = normalize(normal);
    vec3 color = inFairyColor * albedo * max(dot(L, N), 0.0);

    outColor = vec4(color * atten, 1.0);
}
