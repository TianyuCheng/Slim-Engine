#ifndef SLIM_EXAMPLE_VISUALIZE_H
#define SLIM_EXAMPLE_VISUALIZE_H

#include <slim/slim.hpp>

using namespace slim;

struct Visualize {
    RenderGraph::Resource* objectBuffer;
    RenderGraph::Resource* depthBuffer;
    RenderGraph::Resource* surfelcovBuffer;
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


#endif // SLIM_EXAMPLE_VISUALIZE_H
