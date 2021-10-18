#ifndef SLIM_EXAMPLE_UPDATE_H
#define SLIM_EXAMPLE_UPDATE_H

#include <slim/slim.hpp>
using namespace slim;

#include "render.h"
#include "scene.h"

void AddUpdatePass(RenderGraph&       graph,
                   AutoReleasePool&   pool,
                   render::GBuffer*   gbuffer,
                   render::SceneData* sceneData,
                   render::Surfel*    surfel,
                   render::Debug*     debug,
                   Scene*             scene);

#endif // SLIM_EXAMPLE_UPDATE_H
