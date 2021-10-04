#ifndef SLIM_EXAMPLE_GBUFFER_H
#define SLIM_EXAMPLE_GBUFFER_H

#include <slim/slim.hpp>

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
                    AutoReleasePool& bundle,
                    Camera* camera,
                    GBuffer* gbuffer,
                    MainScene* scene);

#endif // SLIM_EXAMPLE_GBUFFER_H
