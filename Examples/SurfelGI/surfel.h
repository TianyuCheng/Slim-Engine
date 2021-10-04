#ifndef SLIM_EXAMPLE_SURFEL_H
#define SLIM_EXAMPLE_SURFEL_H

#include <slim/slim.hpp>
#include <shaderlib/surfel.h>
#include "config.h"
#include "gbuffer.h"
#include "visualize.h"
#include "raytrace.h"

using namespace slim;

struct SurfelState {
    // This holds a value indicating how many surfels
    // are available. It is also used for stack pointer
    // into the array.
    uint32_t surfelStackPointer;

    std::vector<uint32_t> availableSurfelIDs;
};

// ---------------------------------------------------------

class SurfelManager {
public:

    SurfelManager(Device* device, uint32_t numSurfels);

private:
    Device* device;
    uint32_t numSurfels;

    void InitSurfelBuffer();
    void InitSurfelDataBuffer();
    void InitSurfelStatBuffer();
    void InitSurfelGridBuffer();
    void InitSurfelCellBuffer();
    void InitSurfelMomentBuffer();
    void InitSurfelIndirectBuffer();

public:
    SmartPtr<Buffer> surfelBuffer;
    SmartPtr<Buffer> surfelDataBuffer;
    SmartPtr<Buffer> surfelGridBuffer;
    SmartPtr<Buffer> surfelCellBuffer;
    SmartPtr<Buffer> surfelStatBuffer;
    SmartPtr<Buffer> surfelStatBufferCPU;
    SmartPtr<Buffer> surfelIndirectBuffer;

    void ShowSurfelCount(const std::string& prefix, CommandBuffer* commandBuffer);

    RenderGraph::Resource* surfelCovBuffer;
};

// ---------------------------------------------------------

void AddSurfelPass(RenderGraph& renderGraph,
                   AutoReleasePool& pool,
                   Camera* camera,
                   GBuffer* gbuffer,
                   RayTrace* raytrace,
                   Visualize* visualize,
                   SurfelManager* surfel,
                   uint32_t frameId);

#endif // SLIM_EXAMPLE_SURFEL_H
