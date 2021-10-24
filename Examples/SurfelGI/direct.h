#ifndef SLIM_EXAMPLE_DIRECT_H
#define SLIM_EXAMPLE_DIRECT_H

#include <slim/slim.hpp>
using namespace slim;

#include "render.h"
#include "scene.h"

void AddDirectLightingPass(RenderGraph&           graph,
                           AutoReleasePool&       pool,
                           render::GBuffer*       gbuffer,
                           render::SceneData*     sceneData,
                           render::Surfel*        surfel,
                           render::Debug*         debug,
                           Scene*                 scene);

#endif // SLIM_EXAMPLE_DIRECT_H
