#ifndef SLIM_EXAMPLE_COMPOSE_H
#define SLIM_EXAMPLE_COMPOSE_H

#include <slim/slim.hpp>
using namespace slim;

#include "render.h"
#include "scene.h"

void AddComposePass(RenderGraph&           graph,
                    AutoReleasePool&       pool,
                    render::GBuffer*       gbuffer,
                    render::SceneData*     sceneData,
                    render::Surfel*        surfel,
                    render::Debug*         debug,
                    Scene*                 scene,
                    RenderGraph::Resource* colorAttachment);

#endif // SLIM_EXAMPLE_COMPOSE_H
