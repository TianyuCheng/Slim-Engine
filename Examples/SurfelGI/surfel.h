#ifndef SLIM_EXAMPLE_SURFEL_H
#define SLIM_EXAMPLE_SURFEL_H

#include <slim/slim.hpp>
#include <shaderlib/surfel.h>
#include "light.h"
#include "config.h"
#include "gbuffer.h"
#include "scene.h"
#include "visualize.h"

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

    SurfelManager(Device* device);

    accel::AccelStruct* GetTlas() const { return accelBuilder->GetTlas(); }

    void UpdateAABB() const;

private:
    SmartPtr<Device> device;

    SmartPtr<scene::Node> aabbNode;
    SmartPtr<scene::Mesh> aabbMesh;
    SmartPtr<accel::Builder> accelBuilder;

    void InitSurfelBuffer();
    void InitSurfelDataBuffer();
    void InitSurfelStatBuffer();
    void InitSurfelGridBuffer();
    void InitSurfelCellBuffer();
    void InitSurfelAabbBuffer();
    void InitSurfelMomentBuffer();
    void InitSurfelIndirectBuffer();
    void InitSurfelAabbAccelStructure();

public:
    SmartPtr<Buffer> surfelBuffer;
    SmartPtr<Buffer> surfelDataBuffer;
    SmartPtr<Buffer> surfelGridBuffer;
    SmartPtr<Buffer> surfelCellBuffer;
    SmartPtr<Buffer> surfelAabbBuffer;
    SmartPtr<Buffer> surfelStatBuffer;
    SmartPtr<Buffer> surfelStatBufferCPU;
    SmartPtr<Buffer> surfelIndirectBuffer;

    RenderGraph::Resource* surfelCovBuffer;
};

// ---------------------------------------------------------

void AddSurfelPass(RenderGraph& renderGraph,
                   AutoReleasePool& pool,
                   MainScene* scene,
                   GBuffer* gbuffer,
                   Visualize* visualize,
                   SurfelManager* surfel,
                   uint32_t frameId);

#endif // SLIM_EXAMPLE_SURFEL_H
