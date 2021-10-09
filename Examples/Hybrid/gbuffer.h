#ifndef SLIM_EXAMPLE_GBUFFER_H
#define SLIM_EXAMPLE_GBUFFER_H

#include <slim/slim.hpp>
#include "scene.h"

using namespace slim;

class MainScene;

struct GBuffer {
    RenderGraph::Resource* albedoBuffer;
    RenderGraph::Resource* normalBuffer;
    RenderGraph::Resource* positionBuffer;
    RenderGraph::Resource* objectBuffer;
    RenderGraph::Resource* depthBuffer;
};

void AddGBufferPass(RenderGraph& renderGraph,
                    ResourceBundle& bundle,
                    MainScene* scene,
                    GBuffer* gbuffer);

#endif // SLIM_EXAMPLE_GBUFFER_H
