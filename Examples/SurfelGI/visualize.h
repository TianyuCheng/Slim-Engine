#ifndef SLIM_EXAMPLE_VISUALIZE_H
#define SLIM_EXAMPLE_VISUALIZE_H

#include <slim/slim.hpp>

using namespace slim;

struct Visualize {
    RenderGraph::Resource* objectBuffer;
    RenderGraph::Resource* depthBuffer;
    RenderGraph::Resource* surfelCovBuffer;
    RenderGraph::Resource* surfelAllocBuffer;
    RenderGraph::Resource* surfelGridBuffer;
};

void AddObjectVisPass(RenderGraph& renderGraph,
                      AutoReleasePool& pool,
                      RenderGraph::Resource* targetBuffer,
                      RenderGraph::Resource* objectBuffer);

void AddLinearDepthVisPass(RenderGraph& renderGraph,
                           AutoReleasePool& pool,
                           Camera* camera,
                           RenderGraph::Resource* targetBuffer,
                           RenderGraph::Resource* depthBuffer);

void AddSurfelAllocVisPass(RenderGraph& renderGraph,
                           AutoReleasePool& pool,
                           Buffer* buffer,
                           RenderGraph::Resource* targetBuffer);

void AddSurfelGridVisPass(RenderGraph& renderGraph,
                          AutoReleasePool& pool,
                          Camera* camera,
                          RenderGraph::Resource* targetBuffer,
                          RenderGraph::Resource* depthBuffer);


#endif // SLIM_EXAMPLE_VISUALIZE_H
