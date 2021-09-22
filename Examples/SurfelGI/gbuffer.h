#ifndef SLIM_EXAMPLE_GBUFFER_H
#define SLIM_EXAMPLE_GBUFFER_H

#include <slim/slim.hpp>

using namespace slim;

struct GBuffer {
    RenderGraph::Resource* albedoBuffer;
    RenderGraph::Resource* normalBuffer;
    RenderGraph::Resource* positionBuffer;
    RenderGraph::Resource* depthBuffer;
};

void AddGBufferPass(RenderGraph& renderGraph,
                    ResourceBundle& bundle,
                    Camera* camera,
                    GBuffer* gbuffer,
                    scene::Node* scene);

#endif // SLIM_EXAMPLE_GBUFFER_H
