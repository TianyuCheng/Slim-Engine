#version 450
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_separate_shader_objects : enable

// for drawing a quad
const vec2 vertices[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(+1.0, +1.0),
    vec2(+1.0, +1.0),
    vec2(-1.0, +1.0),
    vec2(-1.0, -1.0)
);

const int GAUSSIAN_SAMPLES = 9;

layout(push_constant) uniform Data {
    vec2 singleStepOffset;
    float distanceNormalizationFactor;
} data;

layout(location = 0) out vec2 textureCoordinate;
layout(location = 1) out vec2 blurCoordinates[GAUSSIAN_SAMPLES];

void main()
{
    vec2 pos = vertices[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);
    textureCoordinate = pos * 0.5 + 0.5;
    textureCoordinate.y = -textureCoordinate.y;

    int multiplier = 0;
    vec2 blurStep;

    for (int i = 0; i < GAUSSIAN_SAMPLES; i++) {
        multiplier = (i - ((GAUSSIAN_SAMPLES - 1) / 2));

        blurStep = float(multiplier) * data.singleStepOffset;
        blurCoordinates[i] = textureCoordinate.xy + blurStep;
    }
}
