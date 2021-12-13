#ifndef SLIM_EXAMPLE_RENDER_H
#define SLIM_EXAMPLE_RENDER_H

#include <slim/slim.hpp>
#include "config.h"
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
        RenderGraph::Resource* emissive;
        RenderGraph::Resource* metallicRoughness;
        RenderGraph::Resource* normal;
        RenderGraph::Resource* depth;
        RenderGraph::Resource* object;
        RenderGraph::Resource* globalDiffuse;
        RenderGraph::Resource* directDiffuse;
        RenderGraph::Resource* specular;
        #ifdef ENABLE_GBUFFER_WORLD_POSITION
        RenderGraph::Resource* position;
        #endif
    };

    struct Surfel {
        RenderGraph::Resource* surfels;
        RenderGraph::Resource* surfelLive;
        RenderGraph::Resource* surfelData;
        RenderGraph::Resource* surfelGrid;
        RenderGraph::Resource* surfelCell;
        RenderGraph::Resource* surfelStat;
        RenderGraph::Resource* surfelCoverage;
        RenderGraph::Resource* surfelDepth;
        RenderGraph::Resource* surfelRayGuide;
    };

    struct Debug {
        RenderGraph::Resource* depth;
        RenderGraph::Resource* object;
        RenderGraph::Resource* surfelGrid;
        RenderGraph::Resource* surfelDebug;
        RenderGraph::Resource* surfelBudget;
        RenderGraph::Resource* surfelVariance;
        RenderGraph::Resource* sampleRays;
    };

} // end of render namespace

#endif // SLIM_EXAMPLE_RENDER_H
