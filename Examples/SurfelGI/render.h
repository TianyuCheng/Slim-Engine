#ifndef SLIM_EXAMPLE_RENDER_H
#define SLIM_EXAMPLE_RENDER_H

#include <slim/slim.hpp>
using namespace slim;

namespace render {

    struct SceneData {
        RenderGraph::Resource* lights;
        RenderGraph::Resource* camera;
        RenderGraph::Resource* frame;
    };

    struct GBuffer {
        RenderGraph::Resource* albedo;
        RenderGraph::Resource* normal;
        RenderGraph::Resource* depth;
        RenderGraph::Resource* object;
    };

    struct Surfel {
        RenderGraph::Resource* surfelBuffer;
        RenderGraph::Resource* surfelLiveBuffer;
        RenderGraph::Resource* surfelFreeBuffer;
        RenderGraph::Resource* surfelDataBuffer;
        RenderGraph::Resource* surfelGridBuffer;
        RenderGraph::Resource* surfelCellBuffer;
        RenderGraph::Resource* surfelStatBuffer;
        RenderGraph::Resource* surfelCoverage;
        RenderGraph::Resource* surfelRayGuide;
        RenderGraph::Resource* surfelRadialDepth;
    };

    struct Debug {
        RenderGraph::Resource* depth;
        RenderGraph::Resource* object;
        RenderGraph::Resource* surfelGrid;
        RenderGraph::Resource* surfelDebug;
        RenderGraph::Resource* surfelRayBudget;
        RenderGraph::Resource* surfelAllocation;
    };

} // end of render namespace

#endif // SLIM_EXAMPLE_RENDER_H
