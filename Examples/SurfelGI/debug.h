#ifndef SLIM_EXAMPLE_DEBUG_H
#define SLIM_EXAMPLE_DEBUG_H

#include <slim/slim.hpp>
using namespace slim;

#include "render.h"
#include "scene.h"

void AddLinearDepthPass(RenderGraph&       graph,
                        AutoReleasePool&   pool,
                        render::GBuffer*   gbuffer,
                        render::SceneData* sceneData,
                        render::Surfel*    surfel,
                        render::Debug*     debug,
                        Scene*             scene);

void AddObjectVisPass(RenderGraph&       graph,
                      AutoReleasePool&   pool,
                      render::GBuffer*   gbuffer,
                      render::SceneData* sceneData,
                      render::Surfel*    surfel,
                      render::Debug*     debug,
                      Scene*             scene);

void AddGridVisPass(RenderGraph&       graph,
                    AutoReleasePool&   pool,
                    render::GBuffer*   gbuffer,
                    render::SceneData* sceneData,
                    render::Surfel*    surfel,
                    render::Debug*     debug,
                    Scene*             scene);

void AddSurfelAllocVisPass(RenderGraph&       graph,
                           AutoReleasePool&   pool,
                           render::GBuffer*   gbuffer,
                           render::SceneData* sceneData,
                           render::Surfel*    surfel,
                           render::Debug*     debug,
                           Scene*             scene);

#endif // SLIM_EXAMPLE_DEBUG_H
