#ifndef SLIM_EXAMPLE_OVERLAY_H
#define SLIM_EXAMPLE_OVERLAY_H

#include <slim/slim.hpp>
using namespace slim;

#include "render.h"

class Scene;

void BuildOverlayUI(DearImGui* ui);

void AddOverlayPass(RenderGraph&           graph,
                    AutoReleasePool&       pool,
                    render::GBuffer*       gbuffer,
                    render::SceneData*     sceneData,
                    render::Surfel*        surfel,
                    render::Debug*         debug,
                    Scene*                 scene,
                    DearImGui*             ui,
                    RenderGraph::Resource* colorAttachment);

#endif // SLIM_EXAMPLE_DEBUG_H
