#ifndef SLIM_EXAMPLE_RAYTRACE_H
#define SLIM_EXAMPLE_RAYTRACE_H

#include <slim/slim.hpp>
#include "light.h"
#include "gbuffer.h"

using namespace slim;

void AddRayTracePass(RenderGraph& renderGraph,
                     ResourceBundle& bundle,
                     GBuffer* gbuffer,
                     accel::AccelStruct* tlas,
                     DirectionalLight* light);

#endif // SLIM_EXAMPLE_RAYTRACE_H
