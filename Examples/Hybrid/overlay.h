#ifndef SLIM_EXAMPLE_OVERLAY_H
#define SLIM_EXAMPLE_OVERLAY_H

#include <slim/slim.hpp>
#include "gbuffer.h"

using namespace slim;

void AddOverlayPass(RenderGraph& renderGraph,
                    ResourceBundle& bundle,
                    RenderGraph::Resource* colorBuffer,
                    GBuffer* gbuffer,
                    DearImGui* ui);


#endif // SLIM_EXAMPLE_OVERLAY_H
