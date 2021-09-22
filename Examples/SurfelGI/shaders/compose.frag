#version 450
#extension GL_ARB_separate_shader_objects : enable

// light information
layout(push_constant) uniform DirectionalLight {
    vec4 direction;
    vec4 color;
} dirLight;

layout(set = 0, binding = 0) uniform sampler2D imageAlbedo;
layout(set = 0, binding = 1) uniform sampler2D imageNormalShadow;
layout(set = 0, binding = 2) uniform sampler2D imagePosition;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo = texture(imageAlbedo, inUV).xyz;
    vec4 normal_shadow = texture(imageNormalShadow, inUV);

    // compute very basic diffuse lighting
    vec3 N = normalize(normal_shadow.xyz);
    vec3 L = -normalize(dirLight.direction.xyz);
    vec3 diffuse = albedo * max(0.0, dot(N, L));

    // hack an ambient
    vec3 ambient = albedo * 0.1;

    // apply shadow
    float shadow = normal_shadow.w > 0.0 ? 1.0 : 0.2;

    // final contribution
    outColor = vec4(ambient + diffuse * shadow, 1.0);
}
