#ifndef SLIM_EXAMPLE_RENDER_H
#define SLIM_EXAMPLE_RENDER_H

#include <slim/slim.hpp>
using namespace slim;

namespace render {

    struct SceneData {
        RenderGraph::Resource* sky;
        RenderGraph::Resource* lights;
        RenderGraph::Resource* camera;
        RenderGraph::Resource* frame;
        RenderGraph::Resource* lightXform;
    };

    struct GBuffer {
        RenderGraph::Resource* albedo;
        RenderGraph::Resource* normal;
        RenderGraph::Resource* depth;
        RenderGraph::Resource* object;
    };

    struct Surfel {
        RenderGraph::Resource* surfels;
        RenderGraph::Resource* surfelLive;
        RenderGraph::Resource* surfelData;
        RenderGraph::Resource* surfelGrid;
        RenderGraph::Resource* surfelCell;
        RenderGraph::Resource* surfelStat;
        RenderGraph::Resource* surfelMoment;
        RenderGraph::Resource* surfelDiffuse;
        RenderGraph::Resource* surfelCoverage;
        RenderGraph::Resource* surfelRayGuide;
    };

    struct Debug {
        RenderGraph::Resource* depth;
        RenderGraph::Resource* object;
        RenderGraph::Resource* surfelGrid;
        RenderGraph::Resource* surfelDebug;
        RenderGraph::Resource* surfelBudget;
        RenderGraph::Resource* surfelVariance;
    };

} // end of render namespace

#endif // SLIM_EXAMPLE_RENDER_H
