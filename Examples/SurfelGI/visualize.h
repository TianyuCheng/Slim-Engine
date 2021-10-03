#ifndef SLIM_EXAMPLE_VISUALIZE_H
#define SLIM_EXAMPLE_VISUALIZE_H

#include <slim/slim.hpp>

using namespace slim;

struct Visualize {
    RenderGraph::Resource* objectBuffer;
    RenderGraph::Resource* depthBuffer;
    RenderGraph::Resource* surfelCovBuffer;
    RenderGraph::Resource* surfelAllocBuffer;
};

void AddObjectVisPass(RenderGraph& renderGraph,
                      ResourceBundle& bundle,
                      RenderGraph::Resource* targetBuffer,
                      RenderGraph::Resource* objectBuffer);

void AddLinearDepthVisPass(RenderGraph& renderGraph,
                           ResourceBundle& bundle,
                           Camera* camera,
                           RenderGraph::Resource* targetBuffer,
                           RenderGraph::Resource* depthBuffer);

void AddSurfelAllocVisPass(RenderGraph& renderGraph,
                           ResourceBundle& bundle,
                           Buffer* buffer,
                           RenderGraph::Resource* targetBuffer);


#endif // SLIM_EXAMPLE_VISUALIZE_H
