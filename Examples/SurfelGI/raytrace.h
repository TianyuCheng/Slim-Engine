#ifndef SLIM_EXAMPLE_RAYTRACE_H
#define SLIM_EXAMPLE_RAYTRACE_H

#include <slim/slim.hpp>
#include "light.h"
#include "gbuffer.h"

using namespace slim;

struct RayTrace {
    RenderGraph::Resource* shadowBuffer;  // check if the position is in shadow
    RenderGraph::Resource* surfelBuffer;  // check if the position hits other surfel
};

void AddRayTracePass(RenderGraph& renderGraph,
                     ResourceBundle& bundle,
                     GBuffer* gbuffer,
                     RayTrace* raytrace,
                     accel::AccelStruct* tlas,
                     Camera* camera,
                     DirectionalLight* light);

#endif // SLIM_EXAMPLE_RAYTRACE_H
