#ifndef SLIM_EXAMPLE_SURFEL_H
#define SLIM_EXAMPLE_SURFEL_H

#include <slim/slim.hpp>

using namespace slim;

struct Surfel {
    // basic information of a surfel
    glm::vec3 position   = glm::vec3(0.0);
    glm::vec3 normal     = glm::vec3(0.0);
    glm::vec3 irradiance = glm::vec3(0.0);
    float     radius     = 0.0;

    // This should tell us which surfel it is in the buffer.
    // Modified by compute shader.
    uint32_t  surfelID = 0xffffffff;

    // This should tell us which object this surfel is on.
    // This should also tell us what transform it needs.
    // Modified by compute shader.
    uint32_t  objectID = 0xffffffff;

    // active indicator
    uint32_t  active   = 0;
};

struct SurfelState {
    // This holds a buffer address for surfel array
    uint64_t surfelBaseAddress;

    // This holds a value indicating how many surfels
    // are available. It is also used for stack pointer
    // into the array.
    uint32_t availableSurfels;
};

// ---------------------------------------------------------

class SurfelManager {
public:

    SurfelManager(Device* device, uint32_t numSurfels);

    void SetTransformBuffer(Buffer* transformBuffer);

private:
    Device* device;
    uint32_t numSurfels;

    // This buffer holds surfel manager data
    SmartPtr<Buffer> surfelStateBuffer;

    // This buffer holds all surfel data
    SmartPtr<Buffer> surfelBuffer;

    // This buffer holds a stack of all available surfel IDs
    SmartPtr<Buffer> surfelStackBuffer;

    // This buffer holds all draw information of a surfel (for indirect draw)
    SmartPtr<Buffer> transformBuffer;
};

// ---------------------------------------------------------

void AddSurfelCoveragePass(RenderGraph& renderGraph,
                           ResourceBundle& bundle,
                           Camera* camera,
                           scene::Node* scene);

#endif // SLIM_EXAMPLE_SURFEL_H
