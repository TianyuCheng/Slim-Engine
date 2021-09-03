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
// #include "wavefront.glsl"

hitAttributeEXT vec2 attribs;

// clang-format off
/* layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };             // Positions of an object */
/* layout(buffer_reference, scalar) buffer Indices  { ivec3 i[];  };             // Triangle indices */
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
/* layout(set = 1, binding = 1, scalar) buffer SceneDesc_ { SceneDesc i[]; } sceneDesc; */

layout(location = 0) rayPayloadInEXT HitPayload prd;
// clang-format on

void main()
{
    prd.hitValue = vec3(1.0, 1.0, 1.0); // use a simple solid color for hit

    // // Object data
    // SceneDesc  objResource = sceneDesc.i[gl_InstanceCustomIndexEXT];
    // Indices    indices     = Indices(objResource.indexAddress);
    // Vertices   vertices    = Vertices(objResource.vertexAddress);

    // // Indices of the triangle
    // ivec3 ind = indices.i[gl_PrimitiveID];

    // // Vertex of the triangle
    // Vertex v0 = vertices.v[ind.x];
    // Vertex v1 = vertices.v[ind.y];
    // Vertex v2 = vertices.v[ind.z];

    // const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // // Computing the normal at hit position
    // vec3 normal = v0.nrm * barycentrics.x + v1.nrm * barycentrics.y + v2.nrm * barycentrics.z;
    // // Transforming the normal to world space
    // normal = normalize(vec3(sceneDesc.i[gl_InstanceCustomIndexEXT].transfoIT * vec4(normal, 0.0)));

    // // // Computing the coordinates of the hit position
    // // vec3 worldPos = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
    // // // Transforming the position to world space
    // // worldPos = vec3(sceneDesc.i[gl_InstanceCustomIndexEXT].transfo * vec4(worldPos, 1.0));

    // prd.hitValue = normal;
}
