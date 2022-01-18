#include "white.hlsli"
#include "value.hlsli"
#include "perlin.hlsli"
#include "simplex.hlsli"

struct v2f
{
	float4 pos : SV_POSITION;

	[[vk::location(0)]]
    float2 uv : TEXCOORD0;
};

struct Screen
{
    uint2 resolution;
    float time;
};

struct Control
{
    uint noiseType;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Screen> screen;

[[vk::binding(0, 1)]]
ConstantBuffer<Control> control;

// vertex data for drawing a quad
static float2 vertices[6] =
{
    float2(0.0, 0.0), float2(1.0, 0.0), float2(1.0, 1.0),
    float2(1.0, 1.0), float2(0.0, 1.0), float2(0.0, 0.0)
};

float fbm(float3 st)
{
    // initial values
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 0.;

    #define OCTAVES 6
    // loop of octaves
    for (int i = 0; i < OCTAVES; i++) {
        value += amplitude * noises::simplex::rand1d(st);
        st *= 2.;
        amplitude *= .5;
    }
    return value;
}

// vertex shader
v2f vert(uint vertexID : SV_VertexID)
{
    v2f output;
    output.pos = float4(vertices[vertexID] * 2.0 - 1.0, 0.0, 1.0);
    output.uv = vertices[vertexID];
    return output;
}

// fragment shader
float4 frag(v2f input) : SV_TARGET
{
    float3 uv = float3(input.uv, screen.time * 0.1);

    #if 0 // use white noise directly
    float3 value = noises::white::rand1d(uv);
    #endif

    #if 0 // use value noise with a cell size
    float cellSize = 0.05;
    float3 value = noises::value::rand1d(uv / cellSize);
    #endif

    #if 0 // use perlin noise with a cell size
    float cellSize = 0.05;
    float3 value = noises::perlin::rand1d(uv / cellSize);
    #endif

    #if 0 // use simplex noise with a cell size
    float cellSize = 0.05;
    float3 value = noises::simplex::rand1d(uv / cellSize);
    #endif

    #if 1 // run fractal brownian motion
    float cellSize = 0.05;
    float3 value = fbm(uv / cellSize);
    #endif

    value = value * 0.5 + 0.5;
    float4 color = float4(value, 1.0);
    return color;
}
