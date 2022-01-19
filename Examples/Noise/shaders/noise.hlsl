#include "white.hlsli"
#include "value.hlsli"
#include "perlin.hlsli"
#include "simplex.hlsli"
#include "voronoi.hlsli"

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
    uint dimension;
    float cellSize;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Screen> screen;

[[vk::binding(1, 0)]]
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
    float3 value;

    // use white noise directly
    if (control.noiseType == 0) {
        if (control.dimension == 1) {
            value = noises::white::rand1d(uv);
        }
        else if (control.dimension == 2) {
            value = float3(noises::white::rand2d(uv), 0.0);
        }
        else if (control.dimension == 3) {
            value = noises::white::rand3d(uv);
        }
    }

    // use value noise with a cell size
    if (control.noiseType == 1) {
        if (control.dimension == 1) {
            value = noises::value::rand1d(uv / control.cellSize);
        }
        else if (control.dimension == 2) {
            value = float3(noises::value::rand2d(uv / control.cellSize), 0.0);
        }
        else if (control.dimension == 3) {
            value = noises::value::rand3d(uv / control.cellSize);
        }
    }

    // use perlin noise with a cell size
    if (control.noiseType == 2) {
        if (control.dimension == 1) {
            value = noises::perlin::rand1d(uv / control.cellSize);
        }
        else if (control.dimension == 2) {
            value = float3(noises::perlin::rand2d(uv / control.cellSize), 0.0);
        }
        else if (control.dimension == 3) {
            value = noises::perlin::rand3d(uv / control.cellSize);
        }
    }

    // use simplex noise with a cell size
    if (control.noiseType == 3) {
        if (control.dimension == 1) {
            value = noises::simplex::rand1d(uv / control.cellSize);
        }
        else if (control.dimension == 2) {
            value = float3(noises::simplex::rand2d(uv / control.cellSize), 0.0);
        }
        else if (control.dimension == 3) {
            value = noises::simplex::rand3d(uv / control.cellSize);
        }
    }

    // use voronoi noise with a cell size
    if (control.noiseType == 4) {
        float edgeDist;
        float3 cellPos;
        value = noises::voronoi::rand1d(uv / control.cellSize, cellPos, edgeDist);
        value = noises::white::rand3d(cellPos);
        value *= (1.0 - step(edgeDist, 0.1));
    }

    // run fractal brownian motion
    if (control.noiseType == 5) {
        value = fbm(uv / control.cellSize);
    }

    value = value * 0.5 + 0.5;
    float4 color = float4(value, 1.0);
    return color;
}
