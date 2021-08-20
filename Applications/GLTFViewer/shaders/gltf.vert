#version 450
#extension GL_ARB_separate_shader_objects : enable

// camera set
layout(set = 0, binding = 0) uniform Camera {
    mat4 V;
    mat4 P;
} camera;

// model set
layout(set = 2, binding = 0) uniform Model {
    mat4 M;
    mat3 N;
} model;

// attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTangent;
layout(location = 3) in vec2 inUV0;
layout(location = 4) in vec2 inUV1;
layout(location = 5) in vec4 inColor0;
layout(location = 6) in vec4 joints0;
layout(location = 7) in vec4 weights0;

// varyings
layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outView;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec2 outUV0;
layout(location = 5) out vec2 outUV1;
layout(location = 6) out vec2 outColor0;
layout(location = 7) out vec2 outJoints0;
layout(location = 8) out vec2 outWeights0;

void main() {
    outPosition = vec3(model.M * vec4(inPosition, 1.0));
    outView = vec3(camera.V * vec4(outPosition, 1.0));
    outNormal = mat3(model.N) * inNormal;
    outUV0 = inUV0;
    outUV1 = inUV1;

    gl_Position = camera.P * vec4(outView, 1.0);
}
