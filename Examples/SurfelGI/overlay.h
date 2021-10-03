#ifndef SLIM_EXAMPLE_OVERLAY_H
#define SLIM_EXAMPLE_OVERLAY_H

#include <slim/slim.hpp>
#include "surfel.h"
#include "gbuffer.h"
#include "visualize.h"

using namespace slim;

void AddOverlayPass(RenderGraph& renderGraph,
                    ResourceBundle& bundle,
                    RenderGraph::Resource* colorBuffer,
                    GBuffer* gbuffer,
                    Visualize* visualize,
                    SurfelManager* surfel,
                    DearImGui* ui);


#endif // SLIM_EXAMPLE_OVERLAY_H
