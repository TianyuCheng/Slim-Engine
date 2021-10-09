#ifndef SLIM_EXAMPLE_RAYTRACE_H
#define SLIM_EXAMPLE_RAYTRACE_H

#include <slim/slim.hpp>
#include "scene.h"
#include "light.h"
#include "gbuffer.h"

using namespace slim;

void AddRayTracePass(RenderGraph& renderGraph,
                     ResourceBundle& bundle,
                     MainScene* scene,
                     GBuffer* gbuffer,
                     accel::AccelStruct* tlas);

#endif // SLIM_EXAMPLE_RAYTRACE_H
