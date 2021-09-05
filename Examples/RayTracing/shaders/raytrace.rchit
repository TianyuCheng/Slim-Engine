/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

// This file is copied from NVIDIA's tutorial, with my personal modification.

#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "raycommon.glsl"

struct Vertex
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
};

struct Scene {
    mat4 mvMatrix;
    mat4 nmMatrix;
    uint instanceId;
    uint materialId;
    uint64_t indexAddress;
    uint64_t vertexAddress;
    uint64_t materialAddress;
};

struct Material {
    vec4 baseColor;
    vec4 emissiveColor;
};

hitAttributeEXT vec2 attribs;

// clang-format off
layout(buffer_reference, scalar) buffer Vertices  { Vertex   v[]; };  // Positions of an object
layout(buffer_reference, scalar) buffer Indices   { ivec3    i[];  }; // Triangle indices
layout(buffer_reference, scalar) buffer Materials { Material m[];  }; // material data

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

// camera
layout(set = 1, binding = 0) uniform CameraProperties {
    mat4 view;
    mat4 proj;
    mat4 viewInverse;
    mat4 projInverse;
} cam;

// scene description
layout(set = 1, binding = 1, scalar) buffer Scenes { Scene s[]; } sceneDesc;

layout(location = 0) rayPayloadInEXT HitPayload prd;

// clang-format on

void main()
{
    // Object data
    Scene    scene     = sceneDesc.s[gl_InstanceCustomIndexEXT];
    Indices  indices   = Indices(scene.indexAddress);
    Vertices vertices  = Vertices(scene.vertexAddress);
    Materials material = Materials(scene.materialAddress);

    // Indices of the triangle
    ivec3 ind = indices.i[gl_PrimitiveID];

    // Vertex of the triangle
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // Computing the normal at hit position
    vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    // Transforming the normal to world space
    normal = normalize(vec3(cam.view * scene.nmMatrix * vec4(normal, 0.0)));

    // Computing the coordinates of the hit position
    vec3 worldPos = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    // Transforming the position to world space
    worldPos = vec3(cam.view * scene.mvMatrix * vec4(worldPos, 1.0));

    // assign a simple color
    Material m = material.m[scene.materialId];
    prd.hitValue = m.baseColor.rgb;
}
